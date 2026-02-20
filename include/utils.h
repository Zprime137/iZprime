/**
 * @file utils.h
 * @brief Shared utilities and common includes for the iZprime library.
 *
 * This header centralizes cross-cutting helpers (string checks, arithmetic,
 * formatting, GMP helpers, and system queries).
 */

#ifndef IZ_UTILS_H
#define IZ_UTILS_H

// Standard library includes
#include <stdlib.h>   // For malloc, free, etc.
#include <stdint.h>   // For fixed-width integer types like uint64_t
#include <stddef.h>   // For size_t
#include <string.h>   // For string manipulation functions like snprintf
#include <assert.h>   // For assertions
#include <math.h>     // For math functions like sqrt
#include <stdio.h>    // For printf, FILE, fopen, fwrite, fread, etc.
#include <stdarg.h>   // For variadic functions (va_list, va_start, va_end)
#include <time.h>     // For struct timespec and time-based helpers

#include <platform.h> // OS abstraction layer (process/timing/system helpers)

// Internal includes
#include <logger.h> // Logger module for logging timestamped messages.

// Third-party libraries
#include <gmp.h>         // GMP library for arbitrary precision arithmetic
#include <openssl/sha.h> // For SHA-256 hashing

/** @defgroup iz_utils Utilities
 *  @brief Cross-cutting helpers used across modules.
 *  @{
 */

#ifndef MIN
/** @brief Return the smaller of two expressions. */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
/** @brief Return the larger of two expressions. */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/** Default directory for output artifacts produced by examples/tests. */
#define DIR_output "./output"
/** Convenience macro: number of online CPU cores. */
#define MAX_CORES get_cpu_cores_count()

// file utilities
/**
 * @brief Create a directory if it does not exist.
 * @param dir Directory path.
 * @return 0 on success (including already exists), -1 on failure.
 */
int create_dir(const char *dir);

// string utilities
/**
 * @brief Return non-zero if @p str contains only ASCII digits.
 * @param str Input string.
 * @return 1 if numeric, 0 otherwise.
 */
int is_numeric_str(const char *str);

/**
 * @brief Parse an integer expression into an mpz value.
 *
 * Supported term formats:
 * - plain decimal integer (`1000000`, `1,000,000`)
 * - power notation (`10^6`)
 * - scientific shorthand (`1e6`, `10e100`)
 * - additive expressions (`10e100 + 10e9`)
 *
 * @param out Output mpz value.
 * @param expr Input expression string.
 * @return 1 on success, 0 on parse failure.
 */
int parse_numeric_expr_mpz(mpz_t out, const char *expr);

/**
 * @brief Parse an integer expression into uint64_t.
 * @param expr Input expression string.
 * @param out Output value.
 * @return 1 on success, 0 on parse failure or overflow.
 */
int parse_numeric_expr_u64(const char *expr, uint64_t *out);

/**
 * @brief Parse an inclusive range expression into lower/upper bounds.
 *
 * Accepted forms:
 * - `L,R`
 * - `[L, R]`
 * - `range[L, R]`
 * - `L..R`
 * - `L:R`
 *
 * Each bound accepts the same numeric formats as @ref parse_numeric_expr_mpz.
 *
 * @param range_expr Range expression string.
 * @param lower Output lower bound.
 * @param upper Output upper bound.
 * @return 1 on success, 0 on parse failure.
 */
int parse_inclusive_range_mpz(const char *range_expr, mpz_t lower, mpz_t upper);

// math utilities
/** @brief Compute the greatest common divisor of @p a and @p b. */
uint64_t gcd(uint64_t a, uint64_t b);
/**
 * @brief Compute the modular inverse of @p a modulo @p m.
 * @return The inverse in [0, m-1] if it exists; otherwise an implementation-defined value.
 */
uint64_t modular_inverse(uint64_t a, uint64_t m);

// gmp utilities
/**
 * @brief Seed a GMP random state.
 *
 * Uses platform entropy sources (OpenSSL-backed) with a time-based fallback.
 */
void gmp_seed_randstate(gmp_randstate_t state);

// system utilities
/** @brief Get the number of online CPU cores (>= 1). */
int get_cpu_cores_count(void);
/**
 * @brief Return the CPU L2 cache size in bits (best effort).
 * @return Cache size in bits, or a conservative default.
 */
int get_cpu_L2_cache_size_bits(void);

/** @} */

/** @defgroup print_utils Printer, implemented in toolkit/print_utils.c
 *  @brief Console formatting helpers used by tests and benchmark runners.
 *  @{
 */

/**
 * @brief Print a SHA-256 digest as hex to stdout.
 * @param hash Pointer to the SHA-256 hash array.
 */
void print_sha256_hash(const unsigned char *hash);

/**
 * @brief Print a repeated-character horizontal line.
 * @param length Number of characters to print.
 * @param fill_char Character to repeat (`'-'` is used when zero).
 */
void print_line(int length, char fill_char);

/**
 * @brief Print text centered inside a padded line.
 * @param text Text payload.
 * @param line_length Target total line width.
 * @param fill_char Padding character.
 */
void print_centered_text(const char *text, int line_length, char fill_char);

/** @brief Print the generic test runner header. */
void print_test_table_header(void);

/** @brief Print a test-suite header banner for a module. */
void print_test_module_header(char *module_name);

/** @brief Print a formatted header for a test function. */
void print_test_fn_header(char *fn_name);

/**
 * @brief Print a single test-row result.
 * @param result 1 for pass, 0 for fail.
 * @param test_id Monotonic case index.
 * @param unit_name Short test unit label.
 * @param format `printf`-style detail format.
 */
void print_test_module_result(int result, int test_id, const char *unit_name, const char *format, ...);

/**
 * @brief Print module-level test summary.
 * @param module_name Test module name.
 * @param passed Number of passing tests.
 * @param failed Number of failing tests.
 * @param verbose Non-zero enables richer output hints.
 */
void print_test_summary(char *module_name, int passed, int failed, int verbose);

/** @} */

/** @defgroup iz_stopwatch Stopwatch
 *  @brief Monotonic wall-clock timing helpers for tests and benchmarks.
 *  @{
 */

/**
 * @brief Stopwatch state for elapsed wall-clock measurements.
 */
typedef struct
{
    struct timespec start_time; /**< Captured start timestamp. */
    struct timespec end_time;   /**< Captured stop timestamp. */
    int running;                /**< Non-zero when timing is active. */
    double elapsed_sec;         /**< Elapsed seconds, captured at stop. */
} STOPWATCH;

/**
 * @brief Start or restart a stopwatch.
 * @param sw Stopwatch object.
 */
void sw_start(STOPWATCH *sw);

/**
 * @brief Stop a running stopwatch, capturing the elapsed time in seconds.
 * @param sw Stopwatch object.
 */
void sw_stop(STOPWATCH *sw);

/**
 * @brief Return elapsed seconds for the stopwatch.
 *
 * If the stopwatch is still running, elapsed time is computed against the
 * current monotonic timestamp.
 *
 * @param sw Stopwatch object.
 * @return Elapsed seconds.
 */
double sw_elapsed_seconds(const STOPWATCH *sw);

/**
 * @brief Capture the current monotonic time in seconds.
 * @return Current monotonic timestamp in seconds.
 */
double sw_elapsed_now_seconds(void);

/** @} */

#endif // IZ_UTILS_H
