# Language Bindings Strategy (FFI)

This project exposes language bindings through a stable C-ABI layer in:

- `include/izprime_ffi.h`
- `src/ffi/izprime_ffi.c`

Concrete wrappers live under `bindings/`:

- `bindings/python` (`ctypes`)
- `bindings/rust` (`extern "C"` safe wrapper crate)
- `bindings/go` (`cgo`)
- `bindings/node` (`ffi-napi`)

This avoids binding directly to CLI output, which is slower and brittle for long-term integrations.

## Why API-layer bindings (not CLI)

- Typed arguments/results instead of parsing terminal text.
- Lower overhead (no process spawn for each operation).
- Deterministic memory ownership rules for foreign runtimes.
- Stable function signatures that can be versioned independent of CLI UX.

## FFI API surface (v1)

- `izp_ffi_sieve_u64`:
  run a selected sieve model up to `n <= 10^12`, return all primes as a `uint64_t` buffer.
- `izp_ffi_count_range`:
  count primes in `[start, start + range - 1]`.
- `izp_ffi_stream_range`:
  stream primes (or gaps) in a range to a file.
- `izp_ffi_next_prime`:
  next/previous prime from a base expression.
- `izp_ffi_random_prime_vx`, `izp_ffi_random_prime_vy`:
  random prime generation wrappers.
- `izp_ffi_version`, `izp_ffi_last_error`, `izp_ffi_status_message`:
  runtime/version/error helpers.

## Ownership model

Returned buffers/strings are heap-owned by the library and must be released via:

- `izp_ffi_free_u64_buffer`
- `izp_ffi_free_string`

Do not free returned pointers directly from language runtimes.

## Expression support

`start` and `base` expression inputs use the same parser as CLI/API
(`+ - * / ^ e` with parentheses and grouped decimals).

## Recommended binding rollout

1. Python (`ctypes` or `cffi`) for research/data workflows.
2. Rust (`bindgen` + safe wrapper crate).
3. Node.js (N-API wrapper over C FFI).
4. Go (`cgo` package with thin idiomatic wrappers).

## Notes for wrapper authors

- Treat all FFI return values as status codes (`IZP_FFI_STATUS`).
- On non-`IZP_FFI_OK`, call `izp_ffi_last_error()` for details.
- Keep wrapper-level APIs idiomatic, but keep FFI calls one-to-one and thin.
