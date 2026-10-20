//! RVV 1.0 CRC lookup using two tables held in vector registers.

use super::{Batch, CRCTAB, LANES};
use std::arch::asm;
use std::mem::MaybeUninit;

const fn nibble_table(shift: usize) -> [u32; 16] {
    let mut table = [0; 16];
    let mut i = 0;
    while i < table.len() {
        table[i] = CRCTAB[i << shift];
        i += 1;
    }
    table
}

const LOW: [u32; 16] = nibble_table(0);
const HIGH: [u32; 16] = nibble_table(4);

pub(super) fn crc32(x: Batch, c: Batch) -> Batch {
    let mut result = MaybeUninit::<[u32; LANES]>::uninit();

    // CRC is linear, so CRCTAB[index] = LOW[index & 15] ^ HIGH[index >> 4].
    // e32,m4 holds at least 16 elements on every full V implementation
    // (VLEN >= 128). Load the tables at VL=16, then process the batch in
    // chunks chosen by vsetvli instead of assuming a particular VLEN.
    //
    // SAFETY: this module is compiled only when the V extension is enabled.
    // Inputs and output each span LANES aligned u32 elements. vsetvli picks
    // a nonzero VL <= remaining, so every iteration stays inside them and
    // the loop initializes all output elements. Both gather indices are
    // masked to 0..16, within the initialized portion of the table groups.
    // All registers in the LMUL=4 groups v0..v27 are declared clobbered.
    // Vector configuration may change (we do not use preserves_flags), and
    // all vector instructions leave vstart zero. No rounding state changes.
    unsafe {
        asm!(
            "vsetivli zero, 16, e32, m4, ta, ma",
            "vle32.v v8, ({low})",
            "vle32.v v12, ({high})",
            "2:",
            "vsetvli {vl}, {remaining}, e32, m4, ta, ma",
            "vle32.v v0, ({x})",
            "vle32.v v4, ({c})",
            "vxor.vv v4, v0, v4",
            "vand.vi v16, v4, 15",
            "vrgather.vv v20, v8, v16",
            "vsrl.vi v16, v4, 4",
            "vand.vi v16, v16, 15",
            "vrgather.vv v24, v12, v16",
            "vsrl.vi v0, v0, 8",
            "vxor.vv v0, v0, v20",
            "vxor.vv v0, v0, v24",
            "vse32.v v0, ({result})",
            "slli {bytes}, {vl}, 2",
            "add {x}, {x}, {bytes}",
            "add {c}, {c}, {bytes}",
            "add {result}, {result}, {bytes}",
            "sub {remaining}, {remaining}, {vl}",
            "bnez {remaining}, 2b",
            low = in(reg) LOW.as_ptr(),
            high = in(reg) HIGH.as_ptr(),
            x = inout(reg) x.as_array().as_ptr() => _,
            c = inout(reg) c.as_array().as_ptr() => _,
            result = inout(reg) result.as_mut_ptr().cast::<u32>() => _,
            remaining = inout(reg) LANES => _,
            vl = out(reg) _,
            bytes = out(reg) _,
            out("v0") _, out("v1") _, out("v2") _, out("v3") _,
            out("v4") _, out("v5") _, out("v6") _, out("v7") _,
            out("v8") _, out("v9") _, out("v10") _, out("v11") _,
            out("v12") _, out("v13") _, out("v14") _, out("v15") _,
            out("v16") _, out("v17") _, out("v18") _, out("v19") _,
            out("v20") _, out("v21") _, out("v22") _, out("v23") _,
            out("v24") _, out("v25") _, out("v26") _, out("v27") _,
            options(nostack),
        );
        Batch::from_array(result.assume_init())
    }
}
