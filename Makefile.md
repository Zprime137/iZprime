# Makefile documentation (iZprime)

This document describes the supported `make` targets and configuration variables for this repository.

## Quick start

- Build the main program:
  - `make`
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

- `all` (default): Builds the main binary and runs it
  - Equivalent to: `make directories && make build/src/iZ && make run`
- `run`: Runs the main binary (builds first if needed)
- `debug`: Builds with debug flags (`-O0 -g`) and logging enabled
- `release`: Builds an optimized binary and disables logging
- `clean`: Removes build artifacts and generated output/log directories
- `help`: Prints a concise help summary of targets and options

### Tests

The primary test runner is built at `build/test/test_runner`.

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

### Examples

- `examples`: Builds all example programs into `build/examples/`
- `run-example`: Runs one example
  - Required variables:
    - `EX=<example_name>` (name of the example source without extension)
    - `ARGS="..."` (optional arguments passed to the example)

Example:

- `make run-example EX=sieve_primes ARGS="SiZm 10000000 10"`

## Configuration variables

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

- Redirect test output to a file:
  - `make test-all TEST_OUTPUT=output/test_all.txt`

- Pass extra args to the test runner:
  - `make test-all TEST_ARGS="--help"`

## Notes on outputs

- Build outputs live under `build/`.
- Benchmark/test logs can be redirected via `TEST_OUTPUT`.
- Some benchmarks may be long-running; use `SAVE_RESULTS=1` to persist results when supported by that benchmark.
