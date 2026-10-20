//! Experimental RVV header kernel, selected with --engine rvv.
//!
//! Keep the keys in registers for all eleven header bytes, using native
//! 32-bit indexed loads from the 1 KiB CRC table. The previous per-CRC
//! assembly boundary repeatedly spilled the surrounding SIMD state.

use super::{Batch, SKeyState, CRCTAB, LANES};
use std::arch::asm;
use std::mem::MaybeUninit;

// Two independent streams help cover instruction latency. v0..v2 and v3..v5
// hold their keys, and v6..v9 are scratch. All vector operations use LMUL=1.
macro_rules! keystream_pair {
    () => {
        concat!(
            "vor.vi v6, v2, 3\n",
            "vor.vi v7, v5, 3\n",
            "vxor.vi v8, v6, 1\n",
            "vxor.vi v9, v7, 1\n",
            "vmul.vv v6, v6, v8\n",
            "vmul.vv v7, v7, v9\n",
            "vsrl.vi v6, v6, 8\n",
            "vsrl.vi v7, v7, 8\n",
            "vand.vx v6, v6, {ff}\n",
            "vand.vx v7, v7, {ff}\n",
        )
    };
}

// Inputs v6/v7 are bytes; update the two named CRC states in place.
// vluxei32 uses byte offsets, so mask the CRC byte before multiplying by four.
#[rustfmt::skip]
macro_rules! crc_pair {
    ($a:literal, $b:literal) => {
        concat!(
            "vxor.vv v6, ", $a, ", v6\n",
            "vxor.vv v7, ", $b, ", v7\n",
            "vand.vx v6, v6, {ff}\n",
            "vand.vx v7, v7, {ff}\n",
            "vsll.vi v6, v6, 2\n",
            "vsll.vi v7, v7, 2\n",
            "vluxei32.v v8, ({table}), v6\n",
            "vluxei32.v v9, ({table}), v7\n",
            "vsrl.vi ", $a, ", ", $a, ", 8\n",
            "vsrl.vi ", $b, ", ", $b, ", 8\n",
            "vxor.vv ", $a, ", ", $a, ", v8\n",
            "vxor.vv ", $b, ", ", $b, ", v9\n",
        )
    };
}

pub(super) fn header_keystream(state: &SKeyState, data: &[u8; 11]) -> Batch {
    let mut result = MaybeUninit::<[u32; LANES]>::uninit();
    // SAFETY: this module is compiled only with the full V extension enabled.
    // Each iteration chooses VL <= remaining/2 and processes two adjacent chunks,
    // keeping all accesses within the LANES-element input/output arrays.
    // LANES is even and each iteration consumes 2*VL, so VL stays nonzero and
    // every output is initialized. The inner loop reads exactly 11 bytes.
    // Indexed load offsets are aligned multiples of four in 0..=1020, all
    // inside CRCTAB. No destination overlaps its index. Every modified register
    // is declared, including vector configuration via the default flag
    // clobbers. No rounding state changes, and vstart is zero on exit.
    unsafe {
        asm!(
            "2:",
            "srli {half}, {remaining}, 1",
            "vsetvli {vl}, {half}, e32, m1, ta, ma",
            "slli {bytes}, {vl}, 2",
            "vle32.v v0, ({key0})",
            "add {tmp}, {key0}, {bytes}",
            "vle32.v v3, ({tmp})",
            "add {key0}, {tmp}, {bytes}",
            "vle32.v v1, ({key1})",
            "add {tmp}, {key1}, {bytes}",
            "vle32.v v4, ({tmp})",
            "add {key1}, {tmp}, {bytes}",
            "vle32.v v2, ({key2})",
            "add {tmp}, {key2}, {bytes}",
            "vle32.v v5, ({tmp})",
            "add {key2}, {tmp}, {bytes}",
            "mv {header}, {data}",
            "li {rounds}, 11",
            "3:",
            "lbu {cipher}, 0({header})",
            "addi {header}, {header}, 1",
            keystream_pair!(),
            "vxor.vx v6, v6, {cipher}",
            "vxor.vx v7, v7, {cipher}",
            crc_pair!("v0", "v3"),
            "vand.vx v6, v0, {ff}",
            "vand.vx v7, v3, {ff}",
            "vadd.vv v1, v1, v6",
            "vadd.vv v4, v4, v7",
            "vmul.vx v1, v1, {multiplier}",
            "vmul.vx v4, v4, {multiplier}",
            "vadd.vi v1, v1, 1",
            "vadd.vi v4, v4, 1",
            "vsrl.vi v6, v1, 24",
            "vsrl.vi v7, v4, 24",
            crc_pair!("v2", "v5"),
            "addi {rounds}, {rounds}, -1",
            "bnez {rounds}, 3b",
            keystream_pair!(),
            "vse32.v v6, ({result})",
            "add {result}, {result}, {bytes}",
            "vse32.v v7, ({result})",
            "add {result}, {result}, {bytes}",
            "slli {vl}, {vl}, 1",
            "sub {remaining}, {remaining}, {vl}",
            "bnez {remaining}, 2b",
            table = in(reg) CRCTAB.as_ptr(),
            data = in(reg) data.as_ptr(),
            ff = in(reg) 255usize,
            multiplier = in(reg) 134775813usize,
            key0 = inout(reg) state.key0.as_array().as_ptr() => _,
            key1 = inout(reg) state.key1.as_array().as_ptr() => _,
            key2 = inout(reg) state.key2.as_array().as_ptr() => _,
            result = inout(reg) result.as_mut_ptr().cast::<u32>() => _,
            remaining = inout(reg) LANES => _,
            half = out(reg) _,
            vl = out(reg) _,
            bytes = out(reg) _,
            tmp = out(reg) _,
            header = out(reg) _,
            cipher = out(reg) _,
            rounds = out(reg) _,
            out("v0") _, out("v1") _, out("v2") _, out("v3") _,
            out("v4") _, out("v5") _, out("v6") _, out("v7") _,
            out("v8") _, out("v9") _,
            options(nostack),
        );
        Batch::from_array(result.assume_init())
    }
}
