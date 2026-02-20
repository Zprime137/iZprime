# Makefile documentation (iZprime)

This document describes the modular `make` workflow for this repository.

## Quick start

- Build static library + CLI and run:
  - `make`
- Build library only:
  - `make lib`
- Check dependencies:
  - `make doctor`
- Create release source archive:
  - `make dist VERSION=1.0.0`
- Run unit + integration tests:
  - `make test-all`
- Run sieve benchmarks:
  - `make benchmark-p_sieve`
- Run prime-generation benchmarks:
  - `make benchmark-p_gen`

## Dependencies

- C toolchain: `gcc` or `clang`
- Build tool: `make`
- Libraries:
  - GMP (`-lgmp`)
  - OpenSSL 3 (`-lssl -lcrypto`)

The Makefile prefers `pkg-config` for portability; on macOS it also supports Homebrew path fallbacks.

## Targets

### Build / run

- `all` (default): Builds the CLI and runs it
- `lib`: Builds `build/lib/libizprime.a` and shared-library artifacts (default)
- `cli`: Builds `build/bin/izprime` (or `PROGRAM` override)
- `run`: Runs the CLI binary (builds first if needed)
- `debug`: Builds with debug flags (`-O0 -g`) and logging enabled
- `release`: Builds an optimized binary and disables logging
- `clean`: Removes build artifacts and generated output/log directories
- `help`: Prints a concise help summary of targets and options
- `install`: Installs headers, static/shared libraries, CLI, and `pkg-config` file
- `install-lib`: Installs headers, static/shared libraries, and `pkg-config` file
- `uninstall`: Removes files installed by the targets above
- `doctor`: Verifies build dependencies with a compile/link smoke test
- `dist`: Builds source tarball and SHA256 checksum under `dist/`

### Tests

The primary test runner is built at `build/test/test_runner`.
It links against the static library target (`build/lib/libizprime.a`).

- `test-all`: Runs unit + integration tests
- `test-unit`: Runs unit tests only
- `test-integration`: Runs integration tests only

Notes:

- Test selection defaults to `--unit --integration` when no flags are provided.
- Test runner arguments can be extended via `TEST_ARGS`.

### Benchmarks

Benchmarks are intentionally separate from tests, so they can be invoked explicitly and kept out of typical CI workflows.

- `benchmark-p_sieve`: Runs prime sieve model benchmarks
  - Invokes: `./build/test/test_runner --benchmark-p-sieve ...`
- `benchmark-p_gen`: Runs random prime generation benchmarks
  - Invokes: `./build/test/test_runner --benchmark-p-gen ...`
- `benchmark-SiZ_count`: Runs large-window SiZ_count benchmark
  - Invokes: `./build/test/test_runner --benchmark-siz-count ...`

### Examples

- `examples`: Builds all example programs into `build/examples/`
- `run-example`: Runs one example
  - Required variables:
    - `EX=<example_name>` (name of the example source without extension)
    - `ARGS="..."` (optional arguments passed to the example)

Example:

- `make run-example EX=sieve_primes ARGS="SiZm 10000000 10"`

## Configuration variables

### Local configuration file

- `config.mk` (optional, local)
  - If present, it overrides defaults in `Makefile`.
  - Start from:
    - `cp config.mk.example config.mk`

### Dependency discovery

- `USE_PKG_CONFIG` (default: `1`)
  - When `1`, attempts to use `pkg-config` to discover GMP and OpenSSL.
  - When `0`, skips `pkg-config` and uses the Homebrew/generic fallbacks.

- `PKG_CONFIG` (default: `pkg-config`)
  - Override if you need a specific `pkg-config` binary.

### Build toggles

- `LOGGING` (default: `1`)
  - When `1`, compiles with `-DENABLE_LOGGING`.
  - The `release` target forces `LOGGING=0`.

- `PROGRAM` (default: `izprime`)
  - Output name for the CLI binary under `build/bin/`.

- `CLI_ENTRY` (default: `src/main.c`)
  - Source file used as CLI entrypoint (`main()`).

- `VERSION` (default: `1.0.0`)
  - Release/version metadata for the CLI and shared library.

- `SOVERSION` (default: first component of `VERSION`)
  - Shared-library ABI major version.

- `BUILD_SHARED` (default: `1`)
  - Build shared library artifacts in addition to static library.

### Test/benchmark options

- `VERBOSE` (default: `0`)
  - When `1`, adds `--verbose` to the test runner.

- `SAVE_RESULTS` (default: `0`)
  - When `1`, adds `--save-results` to benchmark runs (where supported).

- `TEST_OUTPUT` (default: empty)
  - When set, redirects test runner output to the given file.

- `TEST_ARGS` (default: empty)
  - Extra arguments appended to the test runner invocation.

## Common recipes

- Run integration tests with verbose output:
  - `make test-integration VERBOSE=1`

- Save benchmark results:
  - `make benchmark-p_sieve SAVE_RESULTS=1`
  - `make benchmark-p_gen SAVE_RESULTS=1`
  - `make benchmark-SiZ_count SAVE_RESULTS=1`

- Redirect test output to a file:
  - `make test-all TEST_OUTPUT=output/test_all.txt`

- Pass extra args to the test runner:
  - `make test-all TEST_ARGS="--help"`

- Install under a local prefix:
  - `make install PREFIX=$HOME/.local`

- Build static-only artifacts:
  - `make lib BUILD_SHARED=0`

- Create a release tarball:
  - `make dist VERSION=1.0.0`

## Notes on outputs

- Build outputs live under `build/`.
- Benchmark/test logs can be redirected via `TEST_OUTPUT`.
- Some benchmarks may be long-running; use `SAVE_RESULTS=1` to persist results when supported by that benchmark.
