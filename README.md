# iZprime

[![CI](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml/badge.svg)](https://github.com/Zprime137/iZprime/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Zprime137/iZprime)](https://github.com/Zprime137/iZprime/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

`iZprime` is a C library and CLI for prime computation at two levels:

1. **Algorithm design level**: clean, single-threaded sieve implementations for study, comparison, and extension.
2. **Practical large-range level**: range counting/streaming and prime search workflows that scale to **arbitrary bounds** (via GMP-backed big integers), not just `2^64`.

If you want a project where you can both *study sieve design* and *run real large-number prime tasks*, iZprime is built for exactly that.

---

## Why iZprime

- Two-track architecture: clean algorithm implementations and pragmatic large-range workflows.
- Arbitrary-bound range support through GMP-backed parsing and arithmetic.
- Reusable toolkit primitives for rapid extension instead of one-off prototypes.
- Strong CI gate across Linux, macOS, and Windows (MSYS2), with warning-clean builds.
- Full documentation path from pseudocode to implementation for serious review.

---

## Quick Start

### 1) Install dependencies

#### macOS (Homebrew)

```bash
brew install gcc make pkg-config gmp openssl@3 doxygen
```

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y build-essential make pkg-config libgmp-dev libssl-dev doxygen
```

#### Windows (MSYS2 UCRT64, experimental)

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-make \
  mingw-w64-ucrt-x86_64-gmp \
  mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-pkgconf
```

### 2) Build and install

```bash
make clean
make cli
sudo make install PREFIX=/usr/local
```

This installs:

- CLI: `izprime`
- headers: `include/*.h`
- library: `libizprime` (static, and shared when enabled)
- pkg-config metadata: `izprime.pc`

If you prefer local execution without install:

```bash
./build/bin/izprime help
```

### 3) Use it immediately

```bash
izprime doctor
izprime stream_primes --range "[0, 10^5]" --print
izprime count_primes --range "[0, 10^9]" --cores-number 8
izprime next_prime --n "10^12 + 39"
izprime is_prime --n "10^100 + 123456789" --rounds 40
```

Numeric input supports: `10^6`, `1e6`, `1,000,000`, and additive expressions like `10^100 + 10^9`.

---

## Why iZprime Is Not Limited to 64-bit Bounds

The algorithmic sieve APIs in `prime_sieve.c` work on classic finite limits (`uint64_t`) and return explicit prime lists. That is intentional for algorithm study.

The practical APIs in `iZ_apps.c` are different:

- range endpoints are parsed from expressions into GMP integers,
- counting/streaming workflows combine deterministic sieving with primality testing,
- operations are designed for ranges like `10^100 + k`.

So while some APIs are `uint64_t` by design, **the library as a system is built for arbitrary-range workflows**.

---

## Developer Guide (Extending the Library)

### Architecture at a glance

- `src/prime_sieve.c`: **algorithmic track**
  - classical and SiZ-family sieve implementations,
  - single-threaded,
  - takes `n`, returns `UI64_ARRAY *`.

- `src/iZ_apps.c`: **pragmatic track**
  - `SiZ_stream`, `SiZ_count`, `next/is_prime/random-prime` workflows,
  - range operations over arbitrary bounds,
  - combines deterministic sieve logic with primality testing.

This separation is deliberate: it keeps algorithm design code readable while keeping production workflows modular.

### Starter toolkit provided by iZprime

You can reuse these instead of re-implementing infrastructure:

- `BITMAP` (`include/bitmap.h`): packed bits, low-level marking/check operations.
- `UI16/32/64_ARRAY` (`include/int_arrays.h`): dynamic typed arrays and helpers.
- `iZ toolkit` (`include/iZ_toolkit.h`):
  - wheel mappings (`iZ`, `iZ_mpz`),
  - wheel/segment construction,
  - solver helpers (`iZm_solve_for_x0`, `iZm_solve_for_x0_mpz`, `iZm_solve_for_y0`),
  - segment lifecycle (`vx_init`, `vx_full_sieve`, `vx_stream`, `vx_free`).

### Memory ownership convention

APIs returning `UI64_ARRAY *` return heap-owned buffers.

```c
UI64_ARRAY *arr = SiZm(1000000);
/* ... */
ui64_free(&arr);
```

### Public API entry points

Primary header: `include/iZ_api.h`

Main groups:

- classic sieves: `SoE`, `SSoE`, `SoEu`, `SoS`, `SoA`
- SiZ family: `SiZ`, `SiZm`, `SiZm_vy`
- range operations: `SiZ_stream`, `SiZ_count`
- prime generation/search: `vy_random_prime`, `vx_random_prime`, `iZ_next_prime`

---

## Study the Algorithms (Pseudocode + Code)

For algorithm design review, start with:

- `docs/pseudocode.pdf`

Then map it to implementation:

- `src/prime_sieve.c` for sieve algorithm mechanics,
- `src/toolkit/iZ_toolkit.c` for wheel/segment internals,
- `src/iZ_apps.c` for large-range operational pipelines.

If you are reviewing for methodology or TOMS-style clarity, this path gives the cleanest flow from specification to implementation.

---

## Build, Test, Benchmark

### Build modes

```bash
make
make debug
make release
make clean
make help
```

### Tests

```bash
make test-all
make test-unit
make test-integration
make test-all verbose
```

### Benchmarks

```bash
make benchmark-p_sieve
make benchmark-p_gen
make benchmark-SiZ_count
```

Save results and auto-plot:

```bash
make benchmark-p_sieve save-results plot
make benchmark-p_gen save-results plot
```

---

## Project Layout

```text
include/         public headers + toolkit interfaces
src/             algorithm implementations + application layer
src/cli/         CLI dispatch and subcommands
examples/        example programs
test/            unit/integration/benchmark runners
docs/            manuals, pseudocode, tests, benchmarks
mk/              modular Makefile fragments
packaging/       packaging and distribution metadata
py_tools/        benchmark plotting utilities
```

---

## Documentation Index

- `docs/cli.md` - CLI commands and examples
- `docs/Makefile.md` - build targets/options
- `docs/tests.md` - test targets and expected output
- `docs/benchmarks.md` - benchmark methodology/results
- `docs/packages.md` - distro packaging plan
- `docs/release.md` - release workflow
- `docs/contribute.md` - contribution checklist
- `docs/pseudocode.pdf` - algorithm pseudocode and design notes

Generate the PDF manual:

```bash
make userManual
```

Output: `docs/userManual.pdf`

---

## Review and Feedback

If you are reviewing iZprime for algorithm quality, correctness, or systems design, start from:

1. `docs/pseudocode.pdf`
2. `src/prime_sieve.c`
3. `src/iZ_apps.c`
4. `docs/benchmarks.md` and `docs/tests.md`

Issues, critical feedback, and contribution PRs are all welcome.

---

## License

MIT License. See `LICENSE`.
