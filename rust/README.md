# Rust version of zipbrute

## Building

Because the code uses SIMD, this needs to be built with `+nightly`.

If you've not installed `nightly` yet, first run:

```
rustup toolchain install nightly
```

Then build with

```
cargo +nightly build --release
```

## Benchmarking

```sh
cargo +nightly run --release -- -f zipdata.txt -l 8
```

The search currently stops after approximately 50 million passwords per cracking
task (`MAX_CHECKS`); this command does not exhaust the eight-character search space.
The release profile already enables `target-cpu=native`.

The optimized search uses 64-lane SIMD batches to hide CRC lookup latency. It
caches each final character's CRC and multiplication contributions, using 64 KiB
per batch to reduce the work of initializing keys for each prefix. On AVX2 targets,
it splits the CRC lookup into three small tables held in registers,
replacing memory gathers with register permutations. On RISC-V, the default
`simd` engine uses portable SIMD lowered by the compiler, including when `+v` is
enabled. Other targets use the portable gather implementation. Keystream
generation uses 16-bit SIMD products because only bits 8..15 are needed. After
checking the first ZIP header in SIMD, it checks only surviving candidates against
the remaining headers using scalar code. Results depend on the CPU and input.

Run correctness tests with `cargo +nightly test` and
`cargo +nightly test --release`.

## Comparing RVV on RISC-V

The hand-written RVV kernel is experimental and requires `--engine rvv`.
The default engine remains `simd`, so enabling `+v` alone does not select the
assembly kernel.

On an RVV 1.0 board such as the Orange Pi RV2, build once and compare both
engines with the same binary, input, and thread count:

```sh
RUSTFLAGS="-C target-cpu=native -C target-feature=+v" cargo +nightly build --release
target/release/rzipbrute -f zipdata.txt -l 8 --engine simd
target/release/rzipbrute -f zipdata.txt -l 8 --engine rvv
```

The RVV kernel keeps two independent sets of keys in registers across all eleven
header bytes. It uses native 32-bit indexed loads from the 1 KiB CRC table, with
LMUL=1 throughout, and adapts to the hardware vector length. This avoids the
large register permutations and repeated state transfers of the previous per-CRC
assembly. Subsequent header checks use the same scalar survivor checks as `simd`.

Correctness is checked against scalar decryption and the SIMD engine under QEMU
at multiple vector lengths. Performance must be measured on the target hardware;
QEMU timings do not establish a speedup.
