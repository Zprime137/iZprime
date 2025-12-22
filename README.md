# iZprime

A high-performance C library for high-scale prime generation tasks.

[![CMake](https://img.shields.io/badge/CMake-4%2B-blue.svg)](https://cmake.org/)
[![GCC](https://img.shields.io/badge/GCC-17%2B-red.svg)](https://gcc.gnu.org/)

## Table of Contents

- [iZprime](#izprime)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
    - [Classic Sieve-iZ Algorithm](#classic-sieve-iz-algorithm)
    - [Segmented Sieve-iZm Algorithm](#segmented-sieve-izm-algorithm)
    - [Sieve-VX Algorithm](#sieve-vx-algorithm)
    - [iZ-Random-Next-Prime Algorithm](#iz-random-next-prime-algorithm)
    - [Random-iZprime Algorithm](#random-izprime-algorithm)
  - [How to Use the Library](#how-to-use-the-library)
    - [Dependencies](#dependencies)
      - [Installing Dependencies](#installing-dependencies)
    - [Compilation](#compilation)
    - [Testing](#testing)
    - [API](#api)
      - [Sieve Methods](#sieve-methods)
      - [Random Prime Generation Methods](#random-prime-generation-methods)
  - [Contributing](#contributing)

## Introduction

**iZprime** is a development toolkit which contains essential data structure modules designed for efficient prime generation.
It includes some of the classic sieve algorithms, as well as a new sieve system, that applies the basic $6x \pm 1$ wheel, combined with other wheels to achieve:

- Significant constant factor improvements to the $O(n \log \log n)$ complexity,
- Constant auxiliary space O(1).

### Classic Sieve-iZ Algorithm

A simple yet effective method that focuses on numbers of the form $6x \pm 1$ (denoted as the iZ set), reducing the search space from $n$ to $\frac{n}{3}$, thereby avoiding a large portion of the redundancies that arise from considering all numbers. This approach requires $O(n/3)$ bits of memory and reduces the composite-marking complexity to

$$\frac{n}{3} \sum^{\sqrt{n}}_{p=5} \frac{1}{p}.$$

### Segmented Sieve-iZm Algorithm

By arranging the iZ set into 2D structures called the iZ-Matrix (iZm), this algorithm decouples the memory footprint from $n$, achieving constant auxiliary space $O(1)$ (requiring as little as 0.8 MB). Furthermore, it skips marking composites for small primes (below 23), further reducing the bit complexity to

$$\frac{n}{3} \sum^{\sqrt{n}}_{p=23} \frac{1}{p}.$$

### Sieve-VX Algorithm

A more practical tool for targeting very large ranges with efficiency. This algorithm combines deterministic and probabilistic sieving; it processes the iZ-Matrix structure with a standardized vector sizes $vx$ defined as the product of the first iZ-primes. For instance, the vector size $vx6$ is defined as:

$$vx6 = 5 \times 7 \times 11 \times 13 \times 17 \times 19 = 1,616,615 \text{\ bits} \approx 0.2 \text{\ MB},$$

spanning a numerical interval

$$S = 6 \times vx6 = 9,699,690.$$

**• Complexity:**

- Requires linear space complexity $O(S/3)$.

- Bounded sieve workload per segment, achieving linear bit complexity $O(n)$ with exceptional constant factors, approximately:

  - $(\frac{1}{2} S)$ rapid bitwise operations.

  - $(\frac{4}{100}S)$ primality testing operations.

**• Output:**

An important feature of this algorithm is that instead of storing unbounded bit-size ($\log_2 p$) per prime, it encodes prime gaps using a compact `uint16_t` (2 bytes) array, significantly reducing the output footprint for large datasets.

### iZ-Random-Next-Prime Algorithm

An efficient method for generating the next/previous prime number relative to a given base. It combines segmented sieving with a probabilistic primality test.

### Random-iZprime Algorithm

A highly efficient method for generating large random primes. It employs a combination of the iZ-Matrix space-filtering techniques and probabilistic primality testing to quickly identify prime candidates of large bit sizes. The algorithm is designed to be fast and scalable by enabling multi-core processing, making it suitable for cryptographic applications where large primes are needed with low latency, such as key generation tasks.

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
pip install numpy matplotlib
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

To execute the tests in _test/test_all.c_, run:

```bash
make test
```

The test module includes high-level test units for ensuring the correctness of the library's functionality:

- `testing_sieve_integrity`: This test invokes the implemented sieve algorithms and passes if all algorithms return the same prime list.

- `testing_sieve_vx`: This test specifically focuses on the `sieve_vx` function. It verifies the correctness of the prime gaps generated by the sieve.

- `testing_vx_io`: This test evaluates the input/output operations of the VX_OBJ structure, ensuring that the serialization and deserialization of prime gaps are functioning correctly.

- `testing_next_prime_gen`: This test checks the functionality of the `iZ_next_prime` and GMP's `mpz_nextprime` functions. It verifies that the generated next prime numbers are correct and consistent with the expected results.

- `testing_prime_gen_algorithms`: This test evaluates the correctness of the `iZ_random_next_prime` and `random_iZprime` functions. It ensures that the generated random primes are valid and meet the specified bit size requirements.

### API

- [`iZ.h`](include/iZ.h) is the primary header file for exposing the API of the iZ-Library. It includes data structure modules (like [`BITMAP`](include/bitmap.h), [`PRIMES_OBJ`](include/primes_obj.h), [`VX_OBJ`](include/vx_obj.h)), iZ utilities and subroutines, prime sieve methods (e.g., [`sieve_iZ`](src/algorithms/sieve_iZ.c), [`sieve_eratosthenes`](src/algorithms/sieve.c)), and random prime generation methods.

#### Sieve Methods

- [`sieve_eratosthenes`]: Optimized Sieve of Eratosthenes algorithm.
- [`segmented_sieve`]: Segmented Sieve of Eratosthenes algorithm.
- [`sieve_euler`]: Sieve of Euler algorithm.
- [`sieve_sundaram`]: Sieve of Sundaram algorithm.
- [`sieve_atkin`]: Sieve of Atkin algorithm.
- [`sieve_iZ`]: Classic Sieve-iZ algorithm.
- [`sieve_iZm`]: Segmented Sieve-iZm algorithm.

**Example usage:**

```c
#include "iZ.h"         // Main library API, it includes all dependencies and necessary headers

int main() {
    uint64_t limit = 1000;
    PRIMES_OBJ *primes_list = NULL;

    // Generate primes using the Classic Sieve-iZ algorithm
    // Other sieve functions like sieve_eratosthenes, etc.,
    // can be called similarly and also return a PRIMES_OBJ*.
    primes_list = sieve_iZ(limit);

    if (primes_list != NULL) {
        printf("Primes up to %llu using sieve_iZ:\n", limit);
        for (uint64_t i = 0; i < primes_list->p_count; i++) {
            printf("%llu ", primes_list->primes[i]);
        }
        printf("\nTotal primes found: %llu\n", primes_list->p_count);

        // Important: Free the memory allocated for the primes object
        // This should be done for any PRIMES_OBJ returned by the sieve functions.
        primes_obj_free(primes_list);
    } else {
        // sieve_iZ returns NULL if n < 10 or if memory allocation fails.
        // Error details are typically logged by the library if the logger is initialized.
        fprintf(stderr, "Failed to generate primes using sieve_iZ (limit: %llu). Check logs or ensure limit >= 10.\n", limit);
    }

    return 0;
}
```

- [`sieve_vx`]: Advanced Sieve-iZm algorithm that processes a VX segment of a specific y in the iZ-Matrix and encodes prime gaps.

**Example usage:**

```c
#include "iZ.h"         // Main library API, it includes all dependencies and necessary headers

int main() {
    int vx6 = 5 * 7 * 11 * 13 * 17 * 19; // Define the segment size as a product of small primes (greater than 3)
    char *y = "10"; // Define the starting y value for the VX segment
    VX_OBJ *vx_obj = vx_init(vx6, y); // Initialize VX_OBJ with a numeric starting y value
    VX_ASSETS *vx_assets = vx_assets_init(vx6); // Initialize reusable VX_ASSETS with the segment size for sieving
    sieve_vx(vx_obj, vx_assets); // Perform the sieve process to obtain prime gaps

    // Print the prime gaps
    printf("Prime gaps in the vx6 segment at y = %s:\n", y);
    vx_print_p_gaps(vx_obj, 10); // Print the first 10 prime gaps detected in the segment

    vx_obj_free(vx_obj); // Free the VX_OBJ after use
    vx_assets_free(vx_assets); // Free the VX_ASSETS after use

    return 0;
}
```

#### Random Prime Generation Methods

- [`random_iZprime`]: Generates a random prime of a specified bit size using the search_iZprime function.
- [`iZ_next_prime`]: Find the next/previous prime number after a given base number.
- [`iZ_random_next_prime`]: Generates a random prime using the iZ_next_prime function.
- [`gmp_random_next_prime`]: Generates a random prime using GMP's mpz_nextprime function invoked on a random base.

**Example usage:**

```c
#include "iZ.h"         // Main library API, it includes all dependencies and necessary headers

int main() {
    mpz_t p1, p2, p3; // Declare mpz_t variables to hold the prime numbers
    mpz_init(p1); // Initialize the variables
    mpz_init(p2);
    mpz_init(p3);

    random_iZprime(p1, 1024, 4); // Sets p1 to a random prime of 1024 bits using 4 cores
    printf("Random prime using random_iZprime: ");
    mpz_out_str(stdout, 10, p1); // Print the random prime in base 10
    printf("\n");

    gmp_random_next_prime(p2, 1024); // Sets p2 to a random prime of 1024 bits using gmp_nextprime on a random base
    printf("Random prime using gmp_random_next_prime: ");
    mpz_out_str(stdout, 10, p2); // Print the random prime in base 10
    printf("\n");
    iZ_random_next_prime(p3, 1024); // Sets p3 to a random prime of 1024 bits using iZ_next_prime on a random base
    printf("Random prime using iZ_random_next_prime: ");
    mpz_out_str(stdout, 10, p3); // Print the random prime in base 10
    printf("\n");

    mpz_clear(p1); // Free the allocated memory for p1
    mpz_clear(p2);
    mpz_clear(p3);

    return 0;
}
```

## Contributing

We welcome contributions from the community! If you have ideas for further optimizations, verified performance improvements, or new features, please feel free to open an issue or submit a pull request. We encourage contributors to include detailed benchmarks and measurements with their proposals to ensure that any changes are rigorously evaluated. Your feedback and contributions will help make the iZ-Library even more robust and efficient for the community.
