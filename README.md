# iZprime

[![CI](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml/badge.svg)](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=Zprime137_iZprime&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=Zprime137_iZprime)
[![Release](https://img.shields.io/github/v/release/Zprime137/iZprime)](https://github.com/Zprime137/iZprime/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

[![DOI](https://zenodo.org/badge/777528709.svg)](https://doi.org/10.5281/zenodo.18839925)

`iZprime` is a C library and CLI for prime enumeration as a mathematical software tool.

It targets two realities at once:

- rigorous algorithm work (clean sieve models, documented pseudocode, reproducible benchmarks),
- practical large-range workflows (counting/streaming/searching primes at arbitrary bounds, not just small native integer ranges).

## Why iZprime

- Prime enumeration in arbitrary ranges: `[start, start + range]`, where `start` can be very large using the `mpz_t` type from GMP.
- Catalog of classic and modern sieve models in one place.
- SiZ family implementations with wheel-aware segmentation.
- Research-friendly code organization with toolkit primitives reusable for new algorithms.
- CLI + library + FFI surface for Python/Rust/Go/Node wrappers.

## Quick Start

### Install

Homebrew (tap):

```bash
brew install Zprime137/izprime/izprime
```

Build from source:

```bash
make doctor
make cli
./build/bin/izprime help
```

### 30-second CLI tour

```bash
izprime doctor
izprime stream_primes --range "[0, 10^6]" --print
izprime stream_primes --range "[10^12, 10^12 + 10^6]" --stream-to output/primes.txt
izprime stream_primes --range "[10^12, 10^12 + 1000]" --print-gaps
izprime count_primes --range "[10^100, 10^100 + 10^9]" --cores max
izprime next_prime --n "10^100 + 123456789"
izprime prev_prime --n "10^100 + 123456789"
izprime is_prime --n "(10^137 + 1) * 3 + 2" --rounds 40
```

Numeric inputs accept grouped integers and expressions with `+ - * / ^ e` and parentheses.

## CLI Commands

`izprime` exposes task-oriented commands over library APIs:

- `stream_primes` (alias: `sieve`) - stream primes or prime gaps over a range.
- `count_primes` (alias: `count`) - count primes in a range.
- `next_prime` (alias: `next`) / `prev_prime` (alias: `prev`) - bi-directional prime search.
- `is_prime` - probabilistic primality testing.
- `test` - model consistency checks.
- `benchmark` - sieve benchmarking.
- `doctor` - runtime/build environment checks.

Use `izprime help <command>` for command options.

## What the Library Provides

### 1) Algorithm catalog (`src/prime_sieve.c`)

Deterministic, single-threaded sieve models with input `n` and full prime-list output.

Classic sieves:

- `SoE` - Sieve of Eratosthenes
- `SSoE` - Segmented Eratosthenes
- `SoS` - Sieve of Sundaram
- `SSoS` - Segmented Sundaram
- `SoEu` - Euler (linear) sieve
- `SoA` - Sieve of Atkin

SiZ family:

- `SiZ` - baseline iZ sieve (`6x-1`, `6x+1` domain)
- `SiZm` - segmented iZm (horizontal)
- `SiZm_vy` - segmented iZm (vertical traversal)

### 2) Practical range/search API (`src/iZ_apps.c`)

- `SiZ_stream` - stream primes (or gaps) in `[start, start + range]`
- `SiZ_count` - count primes in that range
- `iZ_next_prime`, `vx_random_prime`, `vy_random_prime` - prime search/generation

This layer combines deterministic sieving with probabilistic primality checks for scalable workflows.

## Toolkit Foundation

Core reusable modules:

- `BITMAP` (`include/bitmap.h`) - compact candidate marking.
- `UI16_ARRAY`, `UI32_ARRAY`, `UI64_ARRAY` (`include/int_arrays.h`) - dynamic integer arrays with hash/serialization helpers.
- iZ toolkit (`include/iZ_toolkit.h`) - iZ mapping, VX construction, modular hit solvers, segment lifecycle.
- iZm structs: `IZM`, `VX_SEG`, `IZM_RANGE_INFO` for segmented processing and range mapping.

## Public C API

Main entry points:

- `include/iZ_api.h` - native library API.
- `include/izprime_ffi.h` - stable C-ABI surface for bindings.

Minimal C usage:

```c
#include <iZ_api.h>

int main(void) {
    UI64_ARRAY *primes = SiZm(1000000);
    if (!primes) return 1;
    printf("count=%d\n", primes->count);
    ui64_free(&primes);
    return 0;
}
```

## Language Bindings

Wrappers (over `izprime_ffi`, not CLI parsing):

- `bindings/python` (`ctypes`)
- `bindings/rust` (`extern "C"` wrapper crate)
- `bindings/go` (`cgo`)
- `bindings/node` (`ffi-napi`)

See `docs/bindings.md` and `bindings/README.md`.

## Build, Test, Benchmark

Discover all targets:

```bash
make help
```

Build:

```bash
make cli
make lib
```

Tests:

```bash
make test-all
make test-unit
make test-integration
make test-cli
```

Benchmarks:

```bash
make benchmark-p_sieve
make benchmark-p_gen
make benchmark-SiZ_count
```

Save and plot:

```bash
make benchmark-p_sieve save-results plot
make benchmark-p_gen save-results plot
make benchmark-SiZ_count save-results
```

## Dependencies

Core:

- C toolchain (`gcc` or `clang`)
- `make`
- GMP (`gmp`)
- OpenSSL (`libcrypto`)

Optional tooling:

- `pkg-config`
- Python 3 + `matplotlib` (plotting)
- Doxygen + LaTeX (manual generation)

Platform status:

- Linux/macOS: first-class
- Windows: CI-validated via MSYS2/MinGW

## Documentation Map

- `docs/pseudocode.pdf` - formal pseudocode and design rationale
- `docs/userManual.pdf` - Doxygen user manual
- `docs/cli.md` - CLI reference
- `docs/bindings.md` - FFI and bindings strategy
- `docs/benchmarks.md` - methodology and benchmark snapshots
- `docs/tests.md` - test targets and expected outputs
- `docs/Makefile.md` - build system reference
- `docs/contribute.md` - contribution guide

Regenerate docs:

```bash
make userManual
make pseudocode
```

## Project Layout

```text
include/         public headers
src/             algorithms, apps, toolkit, ffi, cli
examples/        standalone C examples
test/            unit/integration/benchmark runners
bindings/        Python/Rust/Go/Node wrappers
docs/            pseudocode/manual/tests/benchmarks
mk/              modular Makefile fragments
packaging/       distro/package metadata
py_tools/        benchmark plotting utilities
```

## Contributing

Contributions are welcome for:

- new sieve models and algorithmic refinements,
- correctness/performance regressions,
- range/search workflow improvements,
- packaging and portability,
- documentation and benchmark reproducibility.

Before opening a PR:

```bash
make test-all
```

## License

MIT License. See `LICENSE`.
