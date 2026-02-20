/**
 * @file iZ_api.h
 * @brief Public API for prime sieving, range scans, and prime generation.
 *
 * This header provides the stable, high-level entry points for the iZ library.
 * Most functions return a heap-allocated `UI64_ARRAY*` that must be released
 * with ui64_free().
 */

#ifndef IZ_API_H
#define IZ_API_H

#include <utils.h>      ///< Common utilities, types, and dependencies.
#include <iZ_toolkit.h> ///< iZ/iZm toolkit structures and helpers.

/** @defgroup iz_api iZ Public API
 *  @brief High-level entry points for sieves and prime generation.
 *
 *  The API is organized into:
 *  - Classic sieves: operate directly on the integer domain.
 *  - SiZ / SiZm: operate in iZ index space (6x-1 and 6x+1), optionally segmented.
 *  - Range variants: count/stream primes over large intervals.
 *  - Prime generators: probabilistic searches using the iZm/VX machinery.
 *  @{
 */

/** @name Classic Prime Sieve Algorithms
 *  @brief Baseline sieves up to a numeric limit.
 *
 * All classic sieve functions take a limit @p n and return an ascending list
 * of primes <= @p n.
 */
///@{

/**
 * @brief Optimized Sieve of Eratosthenes.
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SoE(uint64_t n);

/**
 * @brief Segmented Sieve of Eratosthenes.
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SSoE(uint64_t n);

/**
 * @brief Euler (linear) sieve.
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SoEu(uint64_t n);

/**
 * @brief Sieve of Sundaram.
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SoS(uint64_t n);

/**
 * @brief Sieve of Atkin.
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SoA(uint64_t n);

///@}

/** @name iZ-based Sieve Algorithms
 *  @brief Sieve family operating in the 6x-1 / 6x+1 index space.
 */
///@{

/**
 * @brief Solid Sieve-iZ (wheel 6, iZ index space).
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SiZ(uint64_t n);

/**
 * @brief Segmented Sieve-iZm (VX segmented, horizontal processing).
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SiZm(uint64_t n);

/**
 * @brief Segmented Sieve-iZm (vertical processing; faster, unordered output).
 *
 * This variant emphasizes throughput and may not preserve ascending order of
 * returned primes.
 *
 * @param n Upper bound (inclusive).
 * @return Heap-allocated prime list, or NULL on allocation failure.
 * @pre n is in (10, 10^12].
 */
UI64_ARRAY *SiZm_vy(uint64_t n);

///@}

/** @name SiZ Range Variants
 *  @brief Count/stream primes over a numeric interval.
 */
///@{

/**
 * @brief Input parameters for range sieving/counting.
 *
 * The interval is interpreted as:
 * - Start $Z_s$ from the decimal string `start`.
 * - End $Z_e = Z_s + range - 1$.
 */
typedef struct INPUT_SIEVE_RANGE
{
    char *start;    ///< Start of range as a base-10 numeric string.
    uint64_t range; ///< Interval size (number of integers to cover).
    int mr_rounds;  ///< Miller–Rabin rounds for large primality checks.
    char *filepath; ///< Output path for streaming primes (NULL to disable output).
} INPUT_SIEVE_RANGE;

/**
 * @brief Stream primes in a range to `filepath` (and return the count).
 * @param range Range configuration (must include `filepath`).
 * @return Prime count in the interval, or 0 on error.
 */
uint64_t SiZ_stream(INPUT_SIEVE_RANGE *range);

/**
 * @brief Count primes in a range using multiple worker processes.
 * @param input_range Range configuration (output disabled).
 * @param cores_num Requested worker process count.
 * @return Prime count in the interval, or 0 on error.
 */
uint64_t SiZ_count(INPUT_SIEVE_RANGE *input_range, int cores_num);

///@}

/** @name Prime Generators
 *  @brief Probabilistic prime searches using iZm/VX machinery.
 */
///@{

/**
 * @brief Search for a random prime using vertical (vy) sieving.
 * @param p Output prime (initialized by caller).
 * @param bit_size Target size of @p p in bits.
 * @param cores_num Requested worker processes.
 * @return 1 on success, 0 on failure.
 */
int vy_random_prime(mpz_t p, int bit_size, int cores_num);

/**
 * @brief Search for a random prime using horizontal (vx) sieving.
 * @param p Output prime (initialized by caller).
 * @param bit_size Target size of @p p in bits.
 * @param cores_num Requested worker processes.
 * @return 1 on success, 0 on failure.
 */
int vx_random_prime(mpz_t p, int bit_size, int cores_num);

/**
 * @brief Advance to the next (or previous) prime from a base value.
 * @param p Output prime (initialized by caller).
 * @param base Starting value.
 * @param forward Non-zero to search forward, 0 to search backward.
 * @return 1 on success, 0 on failure.
 */
int iZ_next_prime(mpz_t p, mpz_t base, int forward);

///@}

/** @} */

#endif // IZ_API_H
