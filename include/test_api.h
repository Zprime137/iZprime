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
    int base; /**< Base value in `base^exp` limit notation. */
    int exp;  /**< Exponent value in `base^exp` limit notation. */
} SIEVE_LIMIT;

/** Pair a sieve function with a human-readable name (for reports/benchmarks). */
typedef struct
{
    SIEVE_FN function;   /**< Implementation entry point. */
    const char name[32]; /**< Display name used in test/benchmark output. */
} SIEVE_MODEL;

/** @name Sieve model tests/benchmarks */
///@{
/** @brief Validate utility helpers (numeric/range parsers and shared utilities). */
int TEST_UTILS(int verbose);
/** @brief Run correctness checks across all registered sieve models. */
int TEST_SIEVE_MODELS_INTEGRITY(int verbose);
/** @brief Benchmark registered sieve models and optionally persist results. */
void BENCHMARK_SIEVE_MODELS(int save_results);
///@}

/** @name Range sieve tests/benchmarks */
///@{
/** @brief Validate `SiZ_stream` behavior and output formatting. */
int TEST_SiZ_stream(int verbose);
/** @brief Validate `SiZ_count` correctness across worker counts. */
int TEST_SiZ_count(int verbose);
/** @brief Benchmark `SiZ_count` over increasing ranges starting from 10^10, 10^20, ..., 10^100 using max cores. */
void BENCHMARK_SiZ_count(int save_results);
///@}

/** @name Prime generation tests/benchmarks */
///@{
/** @brief Validate `iZ_next_prime` for forward/backward traversal cases. */
int TEST_iZ_next_prime(int verbose);
/** @brief Validate `vy_random_prime` for primality and bit-length constraints. */
int TEST_vy_random_prime(int verbose);
/** @brief Validate `vx_random_prime` for primality and bit-length constraints. */
int TEST_vx_random_prime(int verbose);

/** @brief Benchmark random-prime generation routines over repeated trials. */
int BENCHMARK_P_GEN_ALGORITHMS(int bit_size, int test_rounds, int save_results);
///@}

/** @} */

#endif // TESTS_H
