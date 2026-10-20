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
replacing memory gathers with register permutations. RVV 1.0 targets built with
`+v` use two 16-entry CRC tables in vector registers and `vrgather.vv` lookups.
The RVV loop adapts to the hardware vector length, including VLEN=128. Other
targets use the portable gather implementation. Keystream generation uses
16-bit SIMD products because only bits 8..15 are needed. After checking the first
ZIP header in SIMD, it checks only surviving candidates against the remaining
headers using scalar code. Results depend on the CPU and input.

Run correctness tests with `cargo +nightly test` and
`cargo +nightly test --release`.
