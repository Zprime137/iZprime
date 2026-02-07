# iZprime — Prime Sieving & Generation Toolkit (C)

Build system: `make` (see [Makefile](Makefile) and [Makefile.md](Makefile.md)).

## Introduction

**iZprime** is a C toolkit for prime sieving and prime generation.

<!-- basic data structure: [bitmap, int_arrays] -->
<!-- sieve modules: [iZm, vx_seg] -->
<!-- sieve algorithms: [SoE, SSoE, SoEu, SoS, SoA, SiZ, SiZm, playground/SiZ_210] -->
<!-- sieve-iZ range variants: SiZ_stream, SiZ_count -->
<!-- prime generators: [vy_random_prime, vx_random_prime, iZ_next_prime, iZ_random_next_prime] -->

## How to Use the Library

### Dependencies

- **Compiler:** GCC or Clang
- **Build System:** Make
- **Libraries:**
  - GMP (GNU Multiple Precision Arithmetic Library)
  - OpenSSL 3 (libssl + libcrypto)

#### Installing Dependencies

**macOS:**

```bash
# Install C dependencies
brew install gcc make gmp openssl@3

# For Python utilities and benchmarking tools
pip install -r py_tools/requirements.txt
```

**Linux:**

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential gcc make
```

```bash
# CentOS/RHEL
sudo yum install gcc make gmp-devel openssl-devel
```

### Compilation

Navigate to the root directory and run:

```bash
make
```

This compiles the project, builds the executable, and runs the entry point `src/main.c`.

### Testing

```bash
make test-all
```

Other useful targets:

```bash
make test-unit
make test-integration
make benchmark-p_sieve
make benchmark-p_gen
```

### API

#### Sieve Methods

Public headers live under `include/`.
