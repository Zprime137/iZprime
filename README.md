# iZprime

[![CI](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml/badge.svg)](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Zprime137/iZprime)](https://github.com/Zprime137/iZprime/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

`iZprime` is a C library + CLI for prime enumeration at arbitrary scales. It includes classic and SiZ-family sieve implementations, as well as application-level tools for counting, streaming, and testing primes in large ranges.

If you need to count, stream, or search primes in ranges such as `10^100 .. 10^100 + 10^9`, this project is built for that.

## Quick Start

### Install

Homebrew (tap):

```bash
brew tap Zprime137/izprime
brew install izprime
```

One-shot:

```bash
brew install Zprime137/izprime/izprime
```

Build from source (Linux/macOS):

```bash
make doctor
make cli
./build/bin/izprime help
```

### CLI commands

```bash
izprime doctor
izprime stream_primes --range "[0, 10^6]" --print
izprime stream_primes --range "[10^12, 10^12 + 10^6]" --stream-to output/primes.txt
izprime count_primes --range "[10^100, 10^100 + 10^9]" --cores-number 8
izprime next_prime --n "10^100 + 123456789"
izprime is_prime --n "(10^61 + 1) * 3 + 2" --rounds 40
```

Numeric inputs accept grouped integers and expressions with `+ - * / ^ e` and parentheses.

## About the Project

`iZprime` is the result of extensive research and development in algorithmic prime enumeration, and is dedicated to serving two audiences through two code paths:

- `src/prime_sieve.c` — **algorithm study and sieve design**
  - single-threaded model implementations,
  - classic and SiZ-family sieve variants,
  - input: `n`, output: full prime list.
- `src/iZ_apps.c` — **practical arbitrary-range operations**
  - `SiZ_stream` and `SiZ_count` for large-range enumeration/counting,
  - deterministic sieve pipeline + primality testing backed by GMP,
  - engineered for operational workloads.

This split is deliberate: one side is for research clarity and extension, the other is for practical computation.

## Algorithms Included

Implemented sieve models (`src/prime_sieve.c`):

- Classic: `SoE`, `SSoE`, `SoEu`, `SoS`, `SoA`
- SiZ family: `SiZ`, `SiZm`, `SiZm_vy`

For the formal algorithm descriptions, complexity analysis, and rationale, see `docs/pseudocode.pdf`.

## Developer Guide (Extending the Library)

### Reusable toolkit modules

- `include/bitmap.h` — compact candidate storage and bit operations.
- `include/int_arrays.h` — dynamic typed arrays (`UI16/32/64_ARRAY`).
- `include/iZ_toolkit.h` — wheel construction, index mapping, modular solvers, VX segment lifecycle.
- `include/iZ_api.h` — public application-level API.

### Typical extension workflow

1. Prototype in `src/playground.c` or a focused new source file.
2. Reuse toolkit primitives (bitmap/wheel/solver/segment) instead of duplicating infrastructure.
3. Promote to API only after behavior stabilizes.
4. Add unit and integration tests.
5. Benchmark against existing implementations.

### Build, test, benchmark

```bash
make help
make test-unit
make test-integration
make benchmark-p_sieve
make benchmark-p_gen
make benchmark-SiZ_count
```

With result capture and plotting:

```bash
make benchmark-p_sieve save-results plot
make benchmark-p_gen save-results plot
```

## Project Layout

```text
include/         public headers and reusable toolkit interfaces
src/             algorithms, apps, and core implementations
src/cli/         CLI dispatcher and subcommands
examples/        direct API usage examples
test/            unit/integration tests and benchmark drivers
docs/            manuals, pseudocode, benchmarks, and test docs
mk/              modular Makefile fragments
packaging/       packaging metadata and release templates
py_tools/        benchmark plotting tools
```

## Documentation Map

- `docs/pseudocode.pdf` — algorithm pseudocode and design notes
- `docs/cli.md` — complete CLI reference
- `docs/benchmarks.md` — benchmark method, results, and plots
- `docs/tests.md` — test targets and expected output
- `docs/Makefile.md` — build system targets/options
- `docs/contribute.md` — contribution workflow

Generate the full user manual PDF:

```bash
make userManual
```

## Contributing

Issues and PRs are welcome, especially for:

- algorithmic improvements,
- correctness/performance regressions,
- portability and packaging,
- documentation and reproducible benchmarks.

Recommended review order for new contributors:

1. `docs/pseudocode.pdf`
2. `src/prime_sieve.c`
3. `src/toolkit/iZ_toolkit.c`
4. `src/iZ_apps.c`
5. `docs/benchmarks.md` and `docs/tests.md`

## License

MIT License. See `LICENSE`.
