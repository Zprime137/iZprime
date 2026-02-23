# iZprime

[![CI](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml/badge.svg)](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Zprime137/iZprime)](https://github.com/Zprime137/iZprime/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

`iZprime` is a modular C library + CLI for two complementary goals:

1. Practical prime workflows (counting, streaming, searching) over very large numeric ranges.
2. Research-friendly sieve implementations with clear internals for extension and experimentation.

Most C sieve libraries are optimized around finite machine bounds (for example `<= 2^64`).
`iZprime` keeps fast bounded sieves, but its application layer is designed for arbitrary-range workflows using expression parsing + GMP-backed arithmetic.

## Quick Start

### Install

Homebrew (tap):

```bash
brew tap Zprime137/izprime
brew install izprime
```

or one-shot:

```bash
brew install Zprime137/izprime/izprime
```

Build from source (macOS example):

```bash
brew install gcc make pkg-config gmp openssl@3 doxygen
make clean
make cli
./build/bin/izprime help
```

Ubuntu/Debian build deps:

```bash
sudo apt-get update
sudo apt-get install -y build-essential make pkg-config libgmp-dev libssl-dev doxygen
```

### Use

```bash
izprime doctor
izprime stream_primes --range "[0, 10^6]" --print
izprime stream_primes --range "[10^12, 10^12 + 10^6]" --stream-to output/primes.txt
izprime stream_primes --range "[0, 1000]" --print-gaps
izprime count_primes --range "[10^100, 10^100 + 10^9]" --cores-number 8
izprime next_prime --n "10^100 + 123456789"
izprime is_prime --n "(10^61 + 1) * 3 + 2" --rounds 40
```

Numeric inputs support grouped integers and arithmetic expressions with `+ - * / ^ e` and parentheses.

## Project Identity

`iZprime` intentionally separates two tracks:

- `src/prime_sieve.c`: algorithm-focused reference implementations.
  - single-threaded model APIs,
  - input: `n`, output: full prime list,
  - ideal for studying sieve design decisions and low-level optimization techniques.

- `src/iZ_apps.c`: pragmatic large-range workflows.
  - `SiZ_stream` and `SiZ_count` for real-world range enumeration/counting,
  - deterministic sieving + primality testing pipeline,
  - designed for ranges beyond native integer limits in practice.

This split is core to the project: one side optimizes understanding and reproducibility, the other optimizes operational utility.

## About the Library

### Algorithms (`src/prime_sieve.c`)

- Classic: `SoE`, `SSoE`, `SoEu`, `SoS`, `SoA`
- SiZ family: `SiZ`, `SiZm`, `SiZm_vy`

These are clean, benchmarkable baselines for sieve algorithm research and implementation comparison.

### Pragmatic APIs (`src/iZ_apps.c`)

- `SiZ_stream`: stream primes (or gaps) in a range and return count
- `SiZ_count`: count primes in a range (multi-process where supported)
- `iZ_next_prime`, `vx_random_prime`, `vy_random_prime`

Primary public entry point: `include/iZ_api.h`.

## Developer Guide (Extending iZprime)

### Starter toolkit you can reuse

- `BITMAP` (`include/bitmap.h`): compact candidate representation and bit ops.
- `UI16/32/64_ARRAY` (`include/int_arrays.h`): dynamic typed arrays.
- `iZ toolkit` (`include/iZ_toolkit.h`):
  - index mappings (`iZ`, `iZ_mpz`),
  - wheel/VX construction and selection,
  - modular hit solvers,
  - VX segment lifecycle (`vx_init`, `vx_full_sieve`, `vx_stream`, `vx_free`).

### Typical extension path

1. Prototype in `src/playground.c` or a focused file under `src/`.
2. Reuse toolkit primitives instead of duplicating infra.
3. Add/adjust API only after behavior is stable.
4. Add unit + integration tests.
5. Benchmark against existing implementations.

### Build, test, benchmark

```bash
make help
make cli
make test-unit
make test-integration
make benchmark-p_sieve
make benchmark-p_gen
make benchmark-SiZ_count
```

With result capture/plot:

```bash
make benchmark-p_sieve save-results plot
make benchmark-p_gen save-results plot
```

## Project Layout

```text
include/         public headers and reusable toolkit interfaces
src/             algorithms, applications, and core implementations
src/cli/         CLI dispatcher and subcommands
examples/        direct API usage examples
test/            unit/integration tests and benchmark drivers
docs/            manuals, benchmarks, tests, and pseudocode
mk/              modular make fragments
packaging/       package metadata and release templates
py_tools/        plotting utilities for benchmark outputs
```

## Documentation

- `docs/pseudocode.pdf`: algorithm pseudocode and design rationale
- `docs/benchmarks.md`: benchmark methodology and result tables/plots
- `docs/tests.md`: test targets and expected pass output
- `docs/cli.md`: full CLI options
- `docs/Makefile.md`: build system details
- `docs/contribute.md`: contribution workflow

Generate user manual PDF:

```bash
make userManual
```

## Contribution

Feedback, performance reports, bug reports, and PRs are welcome.

If you are reviewing the project deeply, the best reading order is:

1. `docs/pseudocode.pdf`
2. `src/prime_sieve.c`
3. `src/toolkit/iZ_toolkit.c`
4. `src/iZ_apps.c`
5. `docs/benchmarks.md` + `docs/tests.md`

## License

MIT License. See `LICENSE`.
