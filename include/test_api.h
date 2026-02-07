/**
 * @file test_api.h
 * @brief Public test/benchmark entry points for the iZ library.
 *
 * Doxygen can include this header when documenting the project to expose the
 * test harness API and benchmark entry points.
 */

#ifndef TESTS_H
#define TESTS_H

/** @defgroup iz_tests Tests and Benchmarks
 *  @brief Test harness entry points.
 *  @{
 */

#include <iZ_api.h>

/** Sieve function type: takes a limit and returns a prime list. */
typedef UI64_ARRAY *(*SIEVE_FN)(uint64_t);

/** Limit specification as $base^{exp}$. */
typedef struct
{
    int base;
    int exp;
} SIEVE_LIMIT;

/** Pair a sieve function with a human-readable name (for reports/benchmarks). */
typedef struct
{
    SIEVE_FN function;
    const char name[32];
} SIEVE_MODEL;

/** @name Sieve model tests/benchmarks */
///@{
int TEST_SIEVE_MODELS_INTEGRITY(int verbose);
void BENCHMARK_SIEVE_MODELS(int save_results);
///@}

/** @name Range sieve tests/benchmarks */
///@{
int TEST_SiZ_stream(int verbose);
int TEST_SiZ_count(int verbose);
///@}

/** @name Prime generation tests/benchmarks */
///@{
int TEST_iZ_next_prime(int verbose);
int TEST_vy_random_prime(int verbose);
int TEST_vx_random_prime(int verbose);

int BENCHMARK_P_GEN_ALGORITHMS(int bit_size, int test_rounds, int save_results);
///@}

/** @} */

#endif // TESTS_H
