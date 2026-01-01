#ifndef TESTS_H
#define TESTS_H

#include <iZ_api.h>

// Sieve function type, takes uint64_t limit and returns a UI64_ARRAY pointer
typedef UI64_ARRAY *(*SIEVE_FN)(uint64_t);

// Structure to define sieve limits in base^exp format
typedef struct
{
    int base;
    int exp;
} SIEVE_LIMIT;

// Structure to associate the sieve function with its name
typedef struct
{
    SIEVE_FN function;
    const char name[32];
} SIEVE_MODEL;

// ========================================================================
// * Testing and Benchmarking Sieve Algorithms
// ========================================================================
int TEST_SIEVE_MODELS_INTEGRITY(int verbose);
void BENCHMARK_SIEVE_MODELS(int save_results);

// ========================================================================
// * Testing and Benchmarking Sieve-iZ-Range Algorithms
// ========================================================================
int TEST_SiZ_stream(int verbose);
int TEST_SiZ_count(int verbose);

// ========================================================================
// * Testing and Benchmarking Random Prime Generation Algorithms
// ========================================================================
int TEST_iZ_next_prime(int verbose);
int TEST_vy_random_prime(int verbose);
int TEST_vx_random_prime(int verbose);

int BENCHMARK_P_GEN_ALGORITHMS(int bit_size, int test_rounds, int save_results);

#endif // TESTS_H
