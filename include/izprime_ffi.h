/**
 * @file izprime_ffi.h
 * @brief Stable C-ABI surface for language bindings.
 *
 * This header exposes a narrow, FFI-safe layer over iZprime's native C API.
 * It is designed for bindings in Python, Rust, Node.js, Go, and similar
 * ecosystems that can call C functions directly.
 */

#ifndef IZPRIME_FFI_H
#define IZPRIME_FFI_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
/*
 * Default to static-link semantics on Windows. This avoids requiring
 * __declspec(dllimport) for in-repo static builds/tests.
 * Define IZP_FFI_USE_DLL in consumers that link against a DLL import lib.
 */
#if defined(IZP_FFI_BUILD)
#define IZP_FFI_API __declspec(dllexport)
#elif defined(IZP_FFI_USE_DLL)
#define IZP_FFI_API __declspec(dllimport)
#else
#define IZP_FFI_API
#endif
#else
#define IZP_FFI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup iz_ffi FFI API
 *  @brief ABI-stable wrapper functions for external language bindings.
 *  @{ */

/** @brief Status/error codes returned by FFI functions. */
typedef enum
{
    IZP_FFI_OK = 0,               /**< Operation completed successfully. */
    IZP_FFI_ERR_INVALID_ARG = 1,  /**< Invalid argument(s). */
    IZP_FFI_ERR_PARSE = 2,        /**< Failed to parse numeric expression/input. */
    IZP_FFI_ERR_ALLOC = 3,        /**< Memory allocation failure. */
    IZP_FFI_ERR_NOT_FOUND = 4,    /**< Requested prime/candidate not found. */
    IZP_FFI_ERR_OPERATION = 5     /**< Underlying API operation failed. */
} IZP_FFI_STATUS;

/** @brief Sieve model selector used by @ref izp_ffi_sieve_u64. */
typedef enum
{
    IZP_SIEVE_SOE = 0,
    IZP_SIEVE_SSOE = 1,
    IZP_SIEVE_SOS = 2,
    IZP_SIEVE_SSOS = 3,
    IZP_SIEVE_SOEU = 4,
    IZP_SIEVE_SOA = 5,
    IZP_SIEVE_SIZ = 6,
    IZP_SIEVE_SIZM = 7,
    IZP_SIEVE_SIZM_VY = 8
} IZP_SIEVE_KIND;

/**
 * @brief Owned buffer of uint64_t values returned by FFI sieve routines.
 *
 * Caller must release with @ref izp_ffi_free_u64_buffer.
 */
typedef struct
{
    uint64_t *data; /**< Heap-allocated values. */
    size_t len;     /**< Number of entries in @p data. */
} IZP_U64_BUFFER;

/** @brief Return the iZprime semantic version string. */
IZP_FFI_API const char *izp_ffi_version(void);

/**
 * @brief Return a human-readable status message for @p code.
 * @param code One of @ref IZP_FFI_STATUS values.
 */
IZP_FFI_API const char *izp_ffi_status_message(int code);

/**
 * @brief Return the last FFI error message from the current process.
 *
 * The returned pointer is owned by the library and valid until the next FFI
 * call that mutates the error state.
 */
IZP_FFI_API const char *izp_ffi_last_error(void);

/** @brief Clear the stored last-error string. */
IZP_FFI_API void izp_ffi_clear_error(void);

/**
 * @brief Run a selected sieve model up to @p limit and return all primes.
 *
 * @param kind Sieve model identifier.
 * @param limit Inclusive upper bound (must satisfy implementation limits).
 * @param out Buffer to populate (caller frees via @ref izp_ffi_free_u64_buffer).
 * @return @ref IZP_FFI_OK on success.
 */
IZP_FFI_API int izp_ffi_sieve_u64(IZP_SIEVE_KIND kind, uint64_t limit, IZP_U64_BUFFER *out);

/**
 * @brief Count primes in [start, start + range - 1].
 *
 * `start` accepts the same numeric expression syntax as the CLI/API parser.
 *
 * @param start Start bound expression.
 * @param range Inclusive interval size.
 * @param mr_rounds Miller-Rabin rounds (clamped by core API).
 * @param cores_num Requested worker-process count.
 * @param out_count Receives total prime count on success.
 */
IZP_FFI_API int izp_ffi_count_range(const char *start, uint64_t range, int mr_rounds, int cores_num, uint64_t *out_count);

/**
 * @brief Stream primes (or gaps) in [start, start + range - 1] to a file.
 *
 * `start` accepts the same numeric expression syntax as the CLI/API parser.
 *
 * @param start Start bound expression.
 * @param range Inclusive interval size.
 * @param mr_rounds Miller-Rabin rounds (clamped by core API).
 * @param filepath Destination file path.
 * @param stream_gaps Non-zero to stream prime gaps instead of prime values.
 * @param out_count Receives total prime count on success.
 */
IZP_FFI_API int izp_ffi_stream_range(const char *start, uint64_t range, int mr_rounds, const char *filepath, int stream_gaps, uint64_t *out_count);

/**
 * @brief Find the next/previous prime from @p base_expr.
 *
 * @param base_expr Base numeric expression.
 * @param forward Non-zero for next prime, zero for previous prime.
 * @param out_prime_base10 Receives heap-allocated decimal string.
 */
IZP_FFI_API int izp_ffi_next_prime(const char *base_expr, int forward, char **out_prime_base10);

/**
 * @brief Generate a random prime using vx-based search.
 * @param bit_size Target prime bit length.
 * @param cores_num Requested worker-process count.
 * @param out_prime_base10 Receives heap-allocated decimal string.
 */
IZP_FFI_API int izp_ffi_random_prime_vx(int bit_size, int cores_num, char **out_prime_base10);

/**
 * @brief Generate a random prime using vy-based search.
 * @param bit_size Target prime bit length.
 * @param cores_num Requested worker-process count.
 * @param out_prime_base10 Receives heap-allocated decimal string.
 */
IZP_FFI_API int izp_ffi_random_prime_vy(int bit_size, int cores_num, char **out_prime_base10);

/**
 * @brief Free a buffer returned by @ref izp_ffi_sieve_u64.
 * @param buffer Address of owned buffer object.
 */
IZP_FFI_API void izp_ffi_free_u64_buffer(IZP_U64_BUFFER *buffer);

/**
 * @brief Free a decimal string returned by FFI prime APIs.
 * @param str Address of owned string pointer.
 */
IZP_FFI_API void izp_ffi_free_string(char **str);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // IZPRIME_FFI_H
