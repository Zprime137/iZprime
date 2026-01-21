# iZprime, A Prime Generation Toolkit

[![CMake](https://img.shields.io/badge/CMake-4%2B-blue.svg)](https://cmake.org/)
[![GCC](https://img.shields.io/badge/GCC-17%2B-red.svg)](https://gcc.gnu.org/)

## Table of Contents

- [iZprime, A Prime Generation Toolkit](#izprime-a-prime-generation-toolkit)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [How to Use the Library](#how-to-use-the-library)
    - [Dependencies](#dependencies)
      - [Installing Dependencies](#installing-dependencies)
    - [Compilation](#compilation)
    - [Testing](#testing)
    - [API](#api)
      - [Sieve Methods](#sieve-methods)

## Introduction

**iZprime** is a C toolkit for efficient prime generation.

<!-- basic data structure: [bitmap, int_arrays] -->
<!-- sieve modules: [iZm, vx_seg] -->
<!-- sieve algorithms: [SoE, SSoE, SoEu, SoS, SoA, SiZ, SiZm, playground/SiZ_210] -->
<!-- sieve-iZ range variants: SiZ_stream, SiZ_count -->
<!-- prime generators: [vy_random_prime, vx_random_prime, iZ_next_prime, iZ_random_next_prime] -->

## How to Use the Library

### Dependencies

- **Compiler:** GCC or Clang

- **Build System:** Make

- **Libraries:** Beside the standard C libraries, the following libraries are required:

  - GMP (GNU Multiple Precision Arithmetic Library) for large integer arithmetic: version 6.2.1

  - OpenSSL for cryptographic functions: version 3.0.7

#### Installing Dependencies

**macOS:**

```bash
# Install C dependencies
brew install gcc make gmp openssl

# For Python utilities and benchmarking tools
pip install matplotlib
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

This compiles the project, builds the executable, and runs the entry point _main.c_.

### Testing

### API

#### Sieve Methods
