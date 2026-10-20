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

On an Intel Core i7-6500U, five alternating runs of the original and optimized
release binaries produced these median search times (excluding compilation):

| Version | Wall time | Passwords/second |
| --- | --- | --- |
| Original | 1.070 s | 46.7 million |
| Optimized | 0.529 s | 94.4 million |

The optimized search uses 64-lane SIMD batches to hide CRC lookup latency. After
checking the first ZIP header in SIMD, it checks only surviving candidates against
the remaining headers using scalar code. Results depend on the CPU and input.

Run correctness tests with `cargo +nightly test` and
`cargo +nightly test --release`.
