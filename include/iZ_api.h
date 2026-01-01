#ifndef IZ_API_H
#define IZ_API_H

#include <utils.h> ///< For utility functions and common includes.

// Data structure modules
#include <iZm.h>    ///< iZ-Matrix related structures and functions.
#include <vx_seg.h> ///< VX segment for encapsulating a segment in the iZ-Matrix structure.

#define N_LIMIT (1000000000000ULL) ///< Maximum sieve limit (10^12) for standard sieve algorithms.

// ========================================================================
// * Classic Prime Sieve Algorithms
// ========================================================================

// Classic sieve of Eratosthenes with established speedups
UI64_ARRAY *SoE(uint64_t n);

// Segmented sieve of Eratosthenes for large ranges
UI64_ARRAY *SSoE(uint64_t n);

// Sieve of Euler algorithm
UI64_ARRAY *SoEu(uint64_t n);

// Sieve of Sundaram algorithm
UI64_ARRAY *SoS(uint64_t n);

// Sieve of Atkin algorithm
UI64_ARRAY *SoA(uint64_t n);

// ========================================================================
// * iZ-based Sieve Algorithms
// ========================================================================
// Basic Sieve-iZ algorithm using the 6x +/- 1 wheel
UI64_ARRAY *SiZ(uint64_t n);

// Segmented Sieve-iZm algorithm using dual-layered wheel factorization
UI64_ARRAY *SiZm(uint64_t n);

typedef struct
{
    char *start;    // start of range (numeric string)
    uint64_t range; // interval size
    int mr_rounds;  // Miller-Rabin rounds
    char *filepath; // path for output results (NULL for no output)
} INPUT_SIEVE_RANGE;

uint64_t SiZ_stream(INPUT_SIEVE_RANGE *range);
uint64_t SiZ_count(INPUT_SIEVE_RANGE *input_range, int cores_num);

// ========================================================================
// * Prime Generators
// ========================================================================
int vy_random_prime(mpz_t p, int bit_size, int cores_num);
int vx_random_prime(mpz_t p, int bit_size, int cores_num);
int iZ_next_prime(mpz_t p, mpz_t base, int forward);

// for benchmarking
int iZ_random_next_prime(mpz_t p, int bit_size);
int gmp_random_next_prime(mpz_t p, int bit_size);

#endif // IZ_API_H
