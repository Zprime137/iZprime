# iZprime

`iZprime` is a performance-oriented C framework for prime sieving and prime generation.

It is designed as a **starting point for research, experimentation, and building real prime‑driven applications**.  
Instead of forcing you to assemble infrastructure from scratch, iZprime provides the essential components that advanced implementations inevitably require — already engineered, tested, and documented.

At a glance, the library gives you:

- compact, production-ready data structures (bitmaps and dynamic integer arrays) with rich utility operations,
- the reusable **iZ / iZm toolkit** (mappings, solvers, wheel construction, segment lifecycle),
- multiple sieve strategies — classical methods plus the SiZ family (`SiZ`, `SiZm`, `SiZm_vy`) — so ideas can be compared and evolved,
- integration tests to validate correctness,
- benchmarking utilities to measure performance and scaling.

All of this is accompanied by extensive documentation so new algorithms can be understood, modified, and extended without reverse‑engineering dense code.

The project intentionally favors **clarity, modularity, and composability**. Advanced wheel and segmentation techniques are exposed through small, well‑defined components so new ideas can be implemented without navigating a maze of tightly coupled optimizations.

The algorithm design and pseudocode are documented in `docs/pseudocode.pdf` and map directly to the implemented code paths, making it straightforward to connect theory, structure, and execution.

## Documentation Map

- `docs/userManual.pdf`: generated API manual (`make userManual`).
- `docs/pseudocode.pdf`: algorithm design and pseudocode documentation.
- `docs/Makefile.md`: Makefile targets, variables, and usage instructions.
- `docs/cli.md`: CLI command reference and examples.
- `docs/benchmarks.md`: benchmark commands, tables, and plots.
- `docs/tests.md`: test targets, execution guidance, and pass snapshots.
- `docs/contribute.md`: contribution standards and PR checklist.
- `docs/release.md`: versioning, changelog, and release process.
- `packaging/README.md`: release/versioning/install conventions.

## Table of Contents

- [iZprime](#izprime)
  - [Documentation Map](#documentation-map)
  - [Table of Contents](#table-of-contents)
  - [1. What the Library Provides](#1-what-the-library-provides)
    - [Classical sieve algorithms](#classical-sieve-algorithms)
    - [SiZ family](#siz-family)
      - [Deterministic sieving implementations](#deterministic-sieving-implementations)
      - [Hybrid sieving implementations](#hybrid-sieving-implementations)
      - [Random prime generation](#random-prime-generation)
      - [Next prime search](#next-prime-search)
  - [2. Core Design](#2-core-design)
  - [3. Project Layout](#3-project-layout)
  - [4. Dependencies](#4-dependencies)
    - [Required](#required)
    - [Optional but recommended](#optional-but-recommended)
    - [Install (macOS, Homebrew)](#install-macos-homebrew)
    - [Install (Ubuntu/Debian)](#install-ubuntudebian)
  - [5. Build and Run](#5-build-and-run)
  - [6. Public API Overview](#6-public-api-overview)
    - [Input limits](#input-limits)
    - [Return ownership rules](#return-ownership-rules)
    - [Range API structure](#range-api-structure)
  - [7. Data Structures](#7-data-structures)
    - [`BITMAP` (`include/bitmap.h`)](#bitmap-includebitmaph)
    - [`UI16_ARRAY`, `UI32_ARRAY`, `UI64_ARRAY` (`include/int_arrays.h`)](#ui16_array-ui32_array-ui64_array-includeint_arraysh)
    - [`IZM` and `VX_SEG` (`include/iZ_toolkit.h`)](#izm-and-vx_seg-includeiz_toolkith)
  - [8. Toolkit Internals (for Extending)](#8-toolkit-internals-for-extending)
  - [9. Usage Examples](#9-usage-examples)
    - [9.1 Prime sieving (library call)](#91-prime-sieving-library-call)
    - [9.2 Range counting/streaming](#92-range-countingstreaming)
    - [9.3 Random prime generation](#93-random-prime-generation)
  - [10. Testing](#10-testing)
  - [11. Benchmarking](#11-benchmarking)
  - [12. Doxygen Documentation](#12-doxygen-documentation)
  - [13. Logging and Output Files](#13-logging-and-output-files)
  - [14. Extension Guidelines](#14-extension-guidelines)
  - [15. Troubleshooting](#15-troubleshooting)
    - [Linker errors for GMP/OpenSSL](#linker-errors-for-gmpopenssl)
    - [`doxygen: command not found`](#doxygen-command-not-found)
    - [Slow or memory-heavy benchmark runs](#slow-or-memory-heavy-benchmark-runs)
  - [16. Contributing](#16-contributing)
  - [17. License](#17-license)

## 1. What the Library Provides

### Classical sieve algorithms

The library provides modern implementations of several classic sieve algorithms:

- `SoE` : Classic Sieve of Eratosthenes
- `SSoE` : Segmented Sieve of Eratosthenes
- `SoEu` : Sieve of Euler (linear sieve)
- `SoS` : Sieve of Sundaram
- `SoA` : Sieve of Atkin

### SiZ family

The SiZ family forms the core of the iZprime framework. It combines wheel factorization with structured traversal strategies to aggressively reduce the constant factors of the $O(N \log \log N)$ cost model while keeping memory usage small enough to stay cache‑efficient.

Instead of committing to a single approach, iZprime provides several compatible SiZ variants built on the same toolkit. This makes it straightforward to select — or experiment with — the strategy that best matches a target range, hardware profile, or application requirement.
It includes:

#### Deterministic sieving implementations

These algorithms return all primes up to `n` in a `UI64_ARRAY`:

- `SiZ` : _Solid_ Sieve-iZ on `6x ± 1`
- `SiZm` : _Segmented_ Sieve-iZm (horizontal sieving)
- `SiZm_vy` : _Segmented_ Sieve-iZm-vy (vertical sieving, unordered output)

All deterministic variants share the same underlying mappings, bitmaps, and solver infrastructure, so improvements in one place typically benefit the others.

#### Hybrid sieving implementations

These algorithms combine deterministic sieving with probabilistic primality testing, operating on an arbitrary range `[start, start + range - 1]`:

- `SiZ_stream` : stream primes in range to file and return count
- `SiZ_count` : count primes in range using multiple processes

#### Random prime generation

Random prime search routines that target a specific bit size and use the iZ/iZm toolkit to efficiently skip non-candidates:

- `vx_random_prime` : random prime search routine, employing horizontal search strategy
- `vy_random_prime` : random prime search routine, employing vertical search strategy

#### Next prime search

- `iZ_next_prime` : next/previous prime near an arbitrary base value

## 2. Core Design

The library is designed around the following core principles:

- **Performance**: algorithms and data structures are optimized for speed and cache efficiency.
- **Reusability**: the iZ/iZm toolkit abstracts common logic behind clean interfaces, allowing new algorithms to be developed without re‑implementing segment management, wheel construction, or solver mechanics.
- **Validation and Benchmarking**: comprehensive tests validate implementations, and benchmarking utilities allow performance to be measured and compared.
- **Documentation**: detailed pseudocode and API documentation ensure users can understand, validate, and extend the library without reverse‑engineering code.

## 3. Project Layout

```text
include/
  iZ_api.h        # public API
  iZ_toolkit.h    # iZ/iZm internal toolkit
  bitmap.h        # packed bitmaps
  int_arrays.h    # dynamic integer arrays
  utils.h         # shared utilities
  logger.h        # logging API
  test_api.h      # test/benchmark entrypoints

src/
  prime_sieve.c   # classical + SiZ sieve implementations
  iZ_apps.c       # range APIs + random-prime APIs
  main.c          # izprime CLI entrypoint
  toolkit/        # bitmap/arrays/utils/logger/toolkit internals

examples/
  sieve_primes.c  # sieve CLI example
  SiZ_range.c     # range count/stream example
  p_genrators.c   # random-prime and next-prime example

test/
  test_all.c      # test runner CLI

docs/
  pseudocode.pdf  # algorithm pseudocode documentation
  cli.md          # CLI command reference
  benchmarks.md   # benchmark commands/results/plots
  tests.md        # test commands/results snapshot
  userManual.pdf  # generated API manual
  ...

packaging/
  izprime.pc.in   # pkg-config template
  README.md       # release/versioning conventions

mk/
  *.mk            # split make fragments (tests/docs/install/package/help)
```

## 4. Dependencies

### Required

- C compiler: GCC or Clang
- `make`
- GMP (`gmp`)
- OpenSSL 3 (`ssl`, `crypto`)

### Optional but recommended

- `pkg-config` (Makefile prefers it for portable dependency discovery)
- Python + `py_tools/requirements.txt` for plotting benchmark outputs
- Doxygen for API/manual generation
- MSYS2 (for Windows builds and test runs)

### Install (macOS, Homebrew)

```bash
brew install gcc make pkg-config gmp openssl@3 doxygen
```

### Install (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential make pkg-config libgmp-dev libssl-dev doxygen
```

### Install (Windows, MSYS2 UCRT64)

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-make \
  mingw-w64-ucrt-x86_64-gmp \
  mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-python-matplotlib
```

Notes:

- Windows support is currently **experimental** and validated via the platform abstraction layer.
- On non-fork platforms, `SiZ_count`, `vx_random_prime`, and `vy_random_prime` transparently fall back to single-process execution.

## 5. Build and Run

From repository root:

```bash
make
```

Notes:

- `make` builds the main binary at `build/bin/izprime` and then runs it.
- `make lib` builds `build/lib/libizprime.a` and shared-library artifacts (`.so`/`.dylib`).
- Main program output is appended to `output/stdout.txt`.

Useful build variants:

```bash
make debug
make release
make doctor
make dist VERSION=1.0.0
make clean
make help
```

### CLI quick start

```bash
./build/bin/izprime help
./build/bin/izprime doctor
./build/bin/izprime count_primes --range "[0, 1e6]" --cores-number 8
./build/bin/izprime stream_primes --range "[0, 1000]" --stream-to output/primes_0_1000.txt
./build/bin/izprime next_prime --n "10^12 + 39"
./build/bin/izprime is_prime --n "10e100 + 10e9" --rounds 40
```

For full command reference, see `docs/cli.md`.

## 6. Public API Overview

Public header: `include/iZ_api.h`

### Input limits

- Most sieve entry points expect: `10 < n <= 10^12`.
- `N_LIMIT` is defined as `1000000000000ULL`.

### Return ownership rules

- Functions returning `UI64_ARRAY *` return heap-owned arrays.
- Caller must release with:

```c
ui64_free(&arr);
```

### Range API structure

```c
typedef struct {
    char *start;      // decimal string
    uint64_t range;   // interval size
    int mr_rounds;    // Miller-Rabin rounds
    char *filepath;   // optional stream output path
} INPUT_SIEVE_RANGE;
```

- `SiZ_stream(&input)` uses `filepath` to stream primes.
- `SiZ_count(&input, cores)` ignores streaming and returns count only.

## 7. Data Structures

### `BITMAP` (`include/bitmap.h`)

Packed bit-array with:

- `size`, `byte_size`, `data`
- optional SHA-256 checksum (`sha256`)

Key operations:

- single-bit set/get/flip/clear
- bulk set/clear
- stepped clearing for sieve marking (`bitmap_clear_steps`, SIMD variant)
- binary I/O + checksum verification

### `UI16_ARRAY`, `UI32_ARRAY`, `UI64_ARRAY` (`include/int_arrays.h`)

Dynamic arrays with:

- capacity/count tracking
- push/pop/resize
- optional SHA-256 verification
- binary read/write helpers

### `IZM` and `VX_SEG` (`include/iZ_toolkit.h`)

Internal SiZm state:

- `IZM`: base VX configuration, root primes, pre-sieved base bitmaps
- `VX_SEG`: one segment runtime state (bitmaps, boundaries, prime counts, operations, optional prime-gap vector)

## 8. Toolkit Internals (for Extending)

Internal toolkit header: `include/iZ_toolkit.h`

Key extension points:

- iZ mapping helpers: `iZ`, `iZ_mpz`
- VX selection: `compute_vx_k`, `compute_l2_vx`, `compute_max_vx`
- first hit solvers:
  - `iZm_solve_for_x0`
  - `iZm_solve_for_x0_mpz`
  - `iZm_solve_for_y0`
- segment lifecycle:
  - `vx_init`
  - `vx_full_sieve`
  - `vx_stream_file`
  - `vx_free`

If you extend SiZ‑family algorithms, this is the layer you should reuse instead of re‑implementing segment logic.

## 9. Usage Examples

### 9.1 Prime sieving (library call)

```c
#include <iZ_api.h>

int main(void) {
    uint64_t n = pow(10, 9);
    UI64_ARRAY *primes = SiZm(n);
    if (!primes) return 1;
    // or use any other sieve method from [SoE, SSoE, SoEu, SoS, SoA, SiZ, SiZm, SiZm_vy]

    printf("count: %d\n", primes->count);
    printf("last prime: %lld\n", primes->array[primes->count - 1]);

    ui64_free(&primes);
    return 0;
}
```

### 9.2 Range counting/streaming

```c
#include <iZ_api.h>

int main(void) {
    INPUT_SIEVE_RANGE in = {
        .start = "1000000000000",
        .range = 1000000,
        .mr_rounds = MR_ROUNDS,
        .filepath = "output/iZ_stream.txt"
    };

    uint64_t count = SiZ_stream(&in);
    // or use count = SiZ_count(&in, cores) for count-only without streaming
    printf("count: %lld\n", count);
    return 0;
}
```

### 9.3 Random prime generation

```c
#include <iZ_api.h>

int main(void) {
    mpz_t p;
    mpz_init(p);

    int ok = vx_random_prime(p, 2048, get_cpu_cores_count());
    if (ok) gmp_printf("random prime: %Zd\n", p);

    mpz_clear(p);
    return ok ? 0 : 1;
}
```

## 10. Testing

For full test workflow, snapshots, and output interpretation, see `docs/tests.md`.

Primary test targets:

```bash
make test-all
make test-unit
make test-integration
```

Verbose output:

```bash
make test-all verbose
make test-unit verbose
make test-integration verbose
```

## 11. Benchmarking

For benchmark methodology, current result tables, and plot artifacts, see `docs/benchmarks.md`.

Dedicated benchmark targets:

```bash
make benchmark-p_sieve
make benchmark-p_gen
make benchmark-SiZ_count
```

With result persistence option:

```bash
make benchmark-p_sieve save-results
make benchmark-p_gen save-results
make benchmark-SiZ_count save-results
```

Run benchmarks and plot automatically:

```bash
make benchmark-p_sieve plot
make benchmark-p_gen plot
```

`benchmark-SiZ_count` currently saves tabular results (no plot integration yet).

Plot manually from terminal (prompts for filepath when omitted):

```bash
python3 py_tools/plot_results.py
```

## 12. Doxygen Documentation

To generate and overwrite `docs/userManual.pdf`, run this command from root directory:

```bash
make userManual
```

## 13. Logging and Output Files

- Logging is enabled by default (`LOGGING=1`, `-DENABLE_LOGGING`).
- Log files are under `logs/`.
- Main program stdout is appended to `output/stdout.txt`.
- Many example/test flows write artifacts under `output/`.

## 14. Extension Guidelines

If you plan to add new algorithms:

1. Add public declarations only to headers in `include/` when API-stable.
2. Keep new internals inside `src/toolkit/` or `src/` as appropriate.
3. Reuse `BITMAP`, `UI*_ARRAY`, `IZM`, and `VX_SEG` before introducing new containers.
4. Preserve ownership conventions (all heap-returning APIs must document free path).
5. Add tests to unit/integration suites and expose benchmark hooks if relevant.
6. Update Doxygen comments so `doxygen Doxyfile` remains coherent.

## 15. Troubleshooting

### Linker errors for GMP/OpenSSL

- Install required dev packages (see [Dependencies](#4-dependencies)).
- Ensure `pkg-config` can resolve `gmp` and OpenSSL.
- On macOS Homebrew, verify `openssl@3` is installed.

### `doxygen: command not found`

Install Doxygen and re-run:

```bash
doxygen Doxyfile
```

### Slow or memory-heavy benchmark runs

- Use smaller limits/bit sizes for local checks.
- Run benchmark targets explicitly (they are intentionally separated from `test-all`).

## 16. Contributing

See `docs/contribute.md` for:
- coding/documentation conventions,
- required validation commands,
- benchmark evidence expectations,
- pull request checklist.

For packaging and release notes workflow, see `docs/release.md`.

## 17. License

This project is licensed under the MIT License. See `LICENSE`.
