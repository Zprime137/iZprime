/**
 * @file utils.h
 * @brief Shared utilities and common includes for the iZ library.
 *
 * This header centralizes cross-cutting helpers (string checks, arithmetic,
 * formatting, GMP helpers, and system queries).
 */

#ifndef UTILS_H
#define UTILS_H

// necessary includes across files

#include <stdio.h>    // For printf, FILE, fopen, fwrite, fread, etc.
#include <stdlib.h>   // For malloc, free, etc.
#include <stdint.h>   // For fixed-width integer types like uint64_t
#include <stddef.h>   // For size_t
#include <string.h>   // For string manipulation functions like snprintf
#include <assert.h>   // For assertions
#include <math.h>     // For math functions like sqrt
#include <sys/stat.h> // For creating directories (mkdir)
#include <fcntl.h>    // For open
#include <signal.h>   // For kill
#include <unistd.h>   // For close
#include <time.h>     // For time
#include <sys/time.h> // For gettimeofday
#include <sys/wait.h> // For waitpid

// Platform-specific includes for sysctl (macOS/BSD)
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <gmp.h>         // GMP library for arbitrary precision arithmetic
#include <openssl/sha.h> // For SHA-256 hashing

#include <logger.h>     // Logger module for logging timestamped messages.
#include <int_arrays.h> // Integer arrays for holding list of numbers and their metadata.
#include <bitmap.h>     // Bitmap data structure for efficient bit manipulation.

/** @defgroup iz_utils Utilities
 *  @brief Cross-cutting helpers used across modules.
 *  @{
 */

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

// math utilities
/** @brief Compute $\gcd(a,b)$. */
uint64_t gcd(uint64_t a, uint64_t b);
/**
 * @brief Compute the modular inverse of @p a modulo @p m.
 * @return The inverse in [0, m-1] if it exists; otherwise an implementation-defined value.
 */
uint64_t modular_inverse(uint64_t a, uint64_t m);

// print utilities
/** @brief Print a repeated character line to stdout. */
void print_line(int length, char fill_char);
/** @brief Print @p text centered within a filled line. */
void print_centered_text(const char *text, int line_length, char fill_char);
/** @brief Print a SHA-256 digest as hex to stdout. */
void print_sha256_hash(const unsigned char *hash);

// gmp utilities
/**
 * @brief Seed a GMP random state.
 *
 * Attempts to seed from `/dev/urandom`, falling back to time-based seeding.
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

// test utils
/** @brief Print a test-suite header banner for a module. */
void print_module_header(char *module_name);
/** @brief Print the generic test runner header. */
void print_test_header(void);

/** @} */

#endif // UTILS_H
