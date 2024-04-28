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

## Other build notes

Maybe `-Zbuild-std` could improve performance?
Setting `RUSTFLAGS="-C target-cpu=native"` could also help.
