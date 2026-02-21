# iZprime

`iZprime` is a high-performance C library and CLI for prime sieving, prime counting over large ranges, and random prime generation.

If you need practical prime infrastructure (not just one sieve implementation), iZprime gives you:

- fast classical + SiZ-family sieve implementations,
- range-oriented APIs for count/stream workflows,
- reusable toolkit internals for extending algorithms,
- built-in tests and benchmark runners,
- documentation that maps design/pseudocode to implementation.

## Start Here (2 minutes)

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
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-python-matplotlib
```

### 2) Build CLI

```bash
make cli
./build/bin/izprime help
```

### 3) Run common tasks

```bash
./build/bin/izprime doctor
./build/bin/izprime stream_primes --range "[0, 1000]" --print
./build/bin/izprime count_primes --range "[0, 1e9]" --cores-number 8
./build/bin/izprime next_prime --n "10^12 + 39"
./build/bin/izprime is_prime --n "10e100 + 10e9" --rounds 40
```

Input parser supports numeric expressions such as `10^6`, `1e6`, `1,000,000`, and additive forms.

## What You Can Do With iZprime

### Sieve algorithms

- `SoE`: Sieve of Eratosthenes
- `SSoE`: Segmented Sieve of Eratosthenes
- `SoEu`: Euler (linear) sieve
- `SoS`: Sundaram sieve
- `SoA`: Atkin sieve
- `SiZ`: solid Sieve-iZ (`6x +/- 1`)
- `SiZm`: segmented Sieve-iZm (horizontal)
- `SiZm_vy`: segmented Sieve-iZm-vy (vertical, unordered output)

### Range operations

- `SiZ_stream`: stream primes in `[start, start + range - 1]` and return count
- `SiZ_count`: count primes in range (multi-process where platform supports `fork`; otherwise single-process fallback)

### Prime generation

- `vx_random_prime`
- `vy_random_prime`
- `iZ_next_prime`

## For Users: Build, Test, Benchmark

### Build targets

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
```

Verbose mode:

```bash
make test-all verbose
```

### Benchmarks

```bash
make benchmark-p_sieve
make benchmark-p_gen
make benchmark-SiZ_count
```

Save results:

```bash
make benchmark-p_sieve save-results
make benchmark-p_gen save-results
make benchmark-SiZ_count save-results
```

Plot results from terminal:

```bash
python3 py_tools/plot_results.py
```

## For Developers

iZprime is designed for extensibility, not only execution speed. Internals are split so new variants can reuse proven components instead of re-implementing them.

### Public API and ownership

Main API header: `include/iZ_api.h`

- Most sieve entry points expect `10 < n <= 10^12`
- `N_LIMIT` is `1000000000000ULL`
- APIs returning `UI64_ARRAY *` are heap-owned; release with:

```c
ui64_free(&arr);
```

Range input contract:

```c
typedef struct {
    char *start;
    uint64_t range;
    int mr_rounds;
    char *filepath;
} INPUT_SIEVE_RANGE;
```

### Core internals

- `BITMAP` (`include/bitmap.h`): packed bits + checksum support
- `UI16/32/64_ARRAY` (`include/int_arrays.h`): dynamic arrays + I/O helpers
- `IZM`, `VX_SEG` (`include/iZ_toolkit.h`): segmented SiZ runtime structures

Useful extension hooks in `include/iZ_toolkit.h`:

- mapping: `iZ`, `iZ_mpz`
- VX selection: `compute_vx_k`, `compute_l2_vx`, `compute_max_vx`
- hit solvers: `iZm_solve_for_x0`, `iZm_solve_for_x0_mpz`, `iZm_solve_for_y0`
- segment lifecycle: `vx_init`, `vx_full_sieve`, `vx_stream`, `vx_free`

### Minimal usage example

```c
#include <iZ_api.h>

int main(void) {
    UI64_ARRAY *primes = SiZm(1000000);
    if (!primes) return 1;

    printf("count: %d\n", primes->count);
    ui64_free(&primes);
    return 0;
}
```

## Documentation

- `docs/cli.md`: CLI reference and command examples
- `docs/Makefile.md`: build-system targets and options
- `docs/tests.md`: test workflow and snapshots
- `docs/benchmarks.md`: benchmark methodology, tables, plots
- `docs/packages.md`: distro-native packaging plan and metadata map
- `docs/pseudocode.pdf`: algorithm design and pseudocode
- `docs/contribute.md`: contribution and PR checklist
- `docs/release.md`: release workflow
- `packaging/README.md`: packaging/versioning/install conventions

Generate API manual PDF:

```bash
make userManual
```

(Generated artifact path: `docs/userManual.pdf`.)

## Project Layout

```text
include/         # public headers + toolkit interfaces
src/             # implementations (algorithms + app layers)
src/cli/         # CLI command dispatch and handlers
examples/        # sample programs
test/            # unit/integration/benchmark runner
docs/            # user-facing project documentation
mk/              # modular make fragments
packaging/       # install/pkg-config/release packaging helpers
```

## License

MIT License. See `LICENSE`.
