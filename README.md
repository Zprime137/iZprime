# iZprime

[![CI](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml/badge.svg)](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Zprime137/iZprime)](https://github.com/Zprime137/iZprime/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

`iZprime` is a modular C library + CLI for prime enumeration and prime-search workflows.

**Key features:**

- a research-friendly catalog of classic and modern sieve implementations,
- pragmatic application APIs for counting/streaming/searching primes in very large ranges,
- a reusable toolkit layer (bitmaps, int arrays, wheel/segment primitives),
- integrated tests, benchmarks, plotting tools, and Doxygen documentation.

## Table of Contents

- [iZprime](#izprime)
  - [Table of Contents](#table-of-contents)
  - [Quick Start](#quick-start)
    - [Install](#install)
    - [First commands](#first-commands)
  - [CLI Overview](#cli-overview)
  - [Project Identity: Two Complementary Paths](#project-identity-two-complementary-paths)
    - [1) Algorithm research path (`src/prime_sieve.c`)](#1-algorithm-research-path-srcprime_sievec)
    - [2) Practical application path (`src/iZ_apps.c`)](#2-practical-application-path-srciz_appsc)
  - [Dependencies](#dependencies)
  - [Algorithms Included](#algorithms-included)
    - [Classic sieves (`src/prime_sieve.c`)](#classic-sieves-srcprime_sievec)
    - [SiZ family (`src/prime_sieve.c`)](#siz-family-srcprime_sievec)
  - [Toolkit and Core Data Structures](#toolkit-and-core-data-structures)
  - [Public API at a Glance](#public-api-at-a-glance)
  - [Build, Test, Benchmark](#build-test-benchmark)
  - [Documentation Map](#documentation-map)
  - [Project Layout](#project-layout)
  - [Typical Extension Workflow](#typical-extension-workflow)
  - [Contributing](#contributing)
  - [License](#license)

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

Build from source:

```bash
make doctor
make cli
./build/bin/izprime help
```

### First commands

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

## CLI Overview

`izprime` exposes task-oriented subcommands over the library API:

- `stream_primes` (alias: `sieve`) — enumerate primes in an inclusive range; can print, stream to file, or print gaps.
- `count_primes` (alias: `count`) — count primes in an inclusive range; supports `--cores N|max`.
- `next_prime` — find the next prime after a value.
- `prev_prime` (alias: `prev`) — find the previous prime before a value.
- `is_prime` — probabilistic primality test.
- `test` — run API-level consistency checks.
- `benchmark` — benchmark sieve models.
- `doctor` — verify runtime/build environment.

Use `izprime help <command>` for command-specific options.

## Project Identity: Two Complementary Paths

### 1) Algorithm research path (`src/prime_sieve.c`)

Deterministic, single-threaded model implementations with a simple interface:

- input: `n`
- output: full prime list up to `n`

This path is ideal for:

- studying sieve design choices,
- comparing model behavior and complexity,
- building new sieve variants in a clean baseline file.

### 2) Practical application path (`src/iZ_apps.c`)

Range-oriented APIs for operational workloads:

- `SiZ_stream` — stream primes or prime gaps in `[start, start + range - 1]`
- `SiZ_count` — count primes over the same range model
- `iZ_next_prime`, `vx_random_prime`, `vy_random_prime` — prime search routines

This path combines deterministic sieving and probabilistic primality checks to stay practical over huge bounds.

## Dependencies

Core dependencies:

- C toolchain (`gcc` or `clang`)
- `make`
- GMP (`gmp`)
- OpenSSL (`libcrypto`)

Recommended/optional tooling:

- `pkg-config` (dependency discovery)
- Python 3 + `matplotlib` (benchmark plots)
- Doxygen + LaTeX (user manual PDF generation)

Platform notes:

- Linux/macOS are first-class build targets.
- Windows CI is validated via MSYS2/MinGW.
- Run `make doctor` to validate the local environment quickly.

## Algorithms Included

### Classic sieves (`src/prime_sieve.c`)

Modern implementations of:

- `SoE` — Sieve of Eratosthenes
- `SSoE` — Segmented Sieve of Eratosthenes
- `SoS` — Sieve of Sundaram
- `SSoS` — Segmented Sieve of Sundaram with wheel optimization "new" (v1.1+)
- `SoEu` — Euler (Linear) Sieve
- `SoA` — Sieve of Atkin
- with more to come..

### SiZ family (`src/prime_sieve.c`)

- `SiZ` — Baseline solid Sieve-iZ (iZ basic wheel $\pm1 \mod 6$)
- `SiZm` — Segmented Sieve-iZm (iZm/vx 2d wheel, horizontal segmentation)
- `SiZm_vy` — Segmented Sieve-iZm-vy (same iZm 2D wheel with vertical/y-major traversal; higher throughput, unordered output)

For formal pseudocode, design rationale, and complexity analysis, read [`docs/pseudocode.pdf`](docs/pseudocode.pdf).
For review/distribution, reference the tagged release artifact on GitHub:
[`releases/latest`](https://github.com/Zprime137/iZprime/releases/latest).

## Toolkit and Core Data Structures

The project is intentionally modular. Key building blocks:

- `BITMAP` (`include/bitmap.h`)
  - compact binary candidate mapping,
  - fast bitwise operations for sieve marking.

- `UI16_ARRAY`, `UI32_ARRAY`, `UI64_ARRAY` (`include/int_arrays.h`)
  - dynamic arrays management with resize/sort/hash/serialization helpers,
  - used throughout sieve/test/benchmark paths.

- iZ toolkit (`include/iZ_toolkit.h`)
  - iZ mapping helpers (`iZ`, `iZ_mpz`),
  - VX sizing and wheel construction,
  - modular hit solvers,
  - segment lifecycle (`vx_init`, `vx_full_sieve`, `vx_stream`, `vx_free`).

- Core toolkit structs (`include/iZ_toolkit.h`)
  - `IZM`: precomputed VX assets (base bitmaps + root primes),
  - `VX_SEG`: per-segment runtime state and counters,
  - `IZM_RANGE_INFO`: maps numeric interval bounds to iZ segment coordinates.

Together, these modules are the extension backbone for both algorithm and application code.

## Public API at a Glance

Main public header: `include/iZ_api.h`

API groups:

- classic sieve models (`SoE`, `SSoE`, `SoS`, `SSoS` "new", `SoEu`, `SoA`)
- SiZ family models (`SiZ`, `SiZm`, `SiZm_vy`)
- range APIs (`SiZ_stream`, `SiZ_count`)
- prime search (`vx_random_prime`, `vy_random_prime`, `iZ_next_prime`)

Minimal example (deterministic sieve):

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

Minimal example (arbitrary-range counting):

```c
#include <iZ_api.h>

int main(void) {
    INPUT_SIEVE_RANGE in = {
        .start = "10^100",
        .range = 1000000,
        .mr_rounds = MR_ROUNDS,
        .filepath = NULL,
        .stream_gaps = 0,
    };

    uint64_t count = SiZ_count(&in, get_cpu_cores_count());
    printf("count=%" PRIu64 "\n", count);
    return 0;
}
```

For complete API details, generate and read the Doxygen manual (`make userManual`).

## Build, Test, Benchmark

Discover all targets:

```bash
make help
```

Build targets:

```bash
make cli
make lib
```

Tests:

```bash
make test-all
make test-unit
make test-integration
```

Verbose test mode:

```bash
make test-all verbose
# or
make -- test-all --verbose
```

Benchmarks:

```bash
make benchmark-p_sieve
make benchmark-p_gen
make benchmark-SiZ_count
```

Save and plot benchmark results:

```bash
make benchmark-p_sieve save-results plot
make benchmark-p_gen save-results plot
make benchmark-SiZ_count save-results
```

Detailed benchmark and test snapshots are tracked in `docs/benchmarks.md` and `docs/tests.md`.

## Documentation Map

- `docs/pseudocode.pdf` — algorithm pseudocode and design rationale
- `docs/cli.md` — full CLI reference
- `docs/benchmarks.md` — benchmark methodology and snapshots
- `docs/tests.md` — test targets, modes, and expected success output
- `docs/Makefile.md` — complete build system documentation
- `docs/contribute.md` — contribution workflow and guidelines

Generate user manual PDF:

```bash
make userManual
```

Generate pseudocode PDF from LaTeX source:

```bash
make pseudocode
```

## Project Layout

```text
include/         public headers and reusable toolkit interfaces
src/             algorithms, applications, and core implementations
src/cli/         CLI command dispatcher and handlers
examples/        standalone API usage examples
test/            unit/integration tests and benchmark drivers
docs/            pseudocode, user manual, tests, and benchmarks
mk/              modular Makefile fragments
packaging/       distribution metadata/templates
py_tools/        benchmark plotting utilities
```

## Typical Extension Workflow

1. Start from `docs/pseudocode.pdf` to align with existing notation and design assumptions.
2. Reuse toolkit modules (`bitmap`, `int_arrays`, iZ/VX helpers) before adding new infra.
3. Prototype in `src/playground.c` or a focused source file.
4. Add tests in `test/tests/` (unit first, then integration).
5. Benchmark against existing models.
6. Promote stable functionality to public API (`include/iZ_api.h`) only after behavior and tests are solid.

Recommended reading order for new contributors:

1. `docs/pseudocode.pdf`
2. `src/prime_sieve.c`
3. `src/toolkit/iZ_toolkit.c`
4. `src/iZ_apps.c`
5. `docs/benchmarks.md` and `docs/tests.md`

## Contributing

Issues and pull requests are welcome, especially for:

- new sieve ideas and algorithmic refinements,
- correctness/performance regression fixes,
- arbitrary-range workflow improvements,
- portability and packaging,
- documentation and benchmark reproducibility.

Before opening a PR, please run:

```bash
make test-all
```

## License

MIT License. See `LICENSE`.
