# iZprime Language Wrappers

These wrappers target the stable C-ABI in:

- `include/izprime_ffi.h`
- `src/ffi/izprime_ffi.c`

## Build prerequisite

Build/install `libizprime` first:

```bash
make lib
```

If the library is not in your loader path, set `IZPRIME_LIB` to the full shared-library path.

## Python (`bindings/python`)

- Implementation: `ctypes` wrapper (`izprime` package).
- Demo:

```bash
PYTHONPATH=bindings/python python3 bindings/python/examples/demo.py
```

## Rust (`bindings/rust`)

- Safe wrapper crate over direct `extern "C"` FFI calls.
- Build example:

```bash
cd bindings/rust
cargo check
```

(Requires linker access to `libizprime` via standard loader paths or env flags.)

## Go (`bindings/go`)

- `cgo` wrapper package with typed helpers.
- Example build:

```bash
cd bindings/go
go test ./...
```

(Requires `libizprime` available to the linker/runtime.)

## Node (`bindings/node`)

- Wrapper using `ffi-napi`.
- Install and run:

```bash
cd bindings/node
npm install
node -e 'const izp=require("./index"); console.log(izp.version())'
```

## Notes

- All wrappers call the same `izprime_ffi` API and inherit its status codes.
- Wrappers should treat non-zero return codes as errors and surface `izp_ffi_last_error()`.
