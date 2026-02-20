/**
 * @file int_arrays.h
 * @brief Dynamic arrays for uint16_t, uint32_t, and uint64_t values.
 *
 * The module provides uniform typed arrays with append/pop operations,
 * deterministic resizing, optional SHA-256 integrity checks, and binary I/O.
 */

#ifndef INT_ARRAYS_H
#define INT_ARRAYS_H

#include <utils.h>

/** @defgroup iz_arrays Integer Arrays
 *  @brief Dynamic integer containers used by sieve and toolkit layers.
 *  @{ */

/** @brief Dynamic array for uint16_t values. */
typedef struct
{
    int capacity;                               /**< Allocated element capacity. */
    int count;                                  /**< Number of valid elements. */
    uint16_t *array;                            /**< Contiguous element storage. */
    int ordered;                                /**< Flag indicating if the array is sorted. */
    unsigned char sha256[SHA256_DIGEST_LENGTH]; /**< SHA-256 over used payload. */
} UI16_ARRAY;

/** @brief Dynamic array for uint32_t values. */
typedef struct
{
    int capacity;                               /**< Allocated element capacity. */
    int count;                                  /**< Number of valid elements. */
    uint32_t *array;                            /**< Contiguous element storage. */
    int ordered;                                /**< Flag indicating if the array is sorted. */
    unsigned char sha256[SHA256_DIGEST_LENGTH]; /**< SHA-256 over used payload. */
} UI32_ARRAY;

/** @brief Dynamic array for uint64_t values. */
typedef struct
{
    int capacity;                               /**< Allocated element capacity. */
    int count;                                  /**< Number of valid elements. */
    uint64_t *array;                            /**< Contiguous element storage. */
    int ordered;                                /**< Flag indicating if the array is sorted. */
    unsigned char sha256[SHA256_DIGEST_LENGTH]; /**< SHA-256 over used payload. */
} UI64_ARRAY;

/** @name UI16 API */
/** @{ */
/** @brief Allocate a UI16 array with an initial capacity. */
UI16_ARRAY *ui16_init(int capacity);
/** @brief Free a UI16 array and null the caller pointer. */
void ui16_free(UI16_ARRAY **array);
/** @brief Resize UI16 storage to @p new_capacity (must be >= count). */
void ui16_resize_to(UI16_ARRAY *array, int new_capacity);
/** @brief Shrink UI16 storage so capacity equals count. */
void ui16_resize_to_fit(UI16_ARRAY *array);
/** @brief Append a uint16 value, growing storage if needed. */
void ui16_push(UI16_ARRAY *array, uint16_t element);
/** @brief Sort values in ascending order. */
void ui16_sort(UI16_ARRAY *array);
/** @brief Remove the last element if the array is non-empty. */
void ui16_pop(UI16_ARRAY *array);
/** @brief Compute SHA-256 checksum over active payload. */
void ui16_compute_hash(UI16_ARRAY *array);
/** @brief Verify the stored checksum against current payload. */
int ui16_verify_hash(UI16_ARRAY *array);
/** @brief Serialize count, payload, and checksum to a binary stream. */
int ui16_fwrite(UI16_ARRAY *array, FILE *file);
/** @brief Deserialize a UI16 array from a binary stream. */
UI16_ARRAY *ui16_fread(FILE *file);
/** @brief Execute UI16 test suite. */
int TEST_UI16_ARRAY(int verbose);
/** @} */

/** @name UI32 API */
/** @{ */
/** @brief Allocate a UI32 array with an initial capacity. */
UI32_ARRAY *ui32_init(int capacity);
/** @brief Free a UI32 array and null the caller pointer. */
void ui32_free(UI32_ARRAY **array);
/** @brief Resize UI32 storage to @p new_capacity (must be >= count). */
void ui32_resize_to(UI32_ARRAY *array, int new_capacity);
/** @brief Shrink UI32 storage so capacity equals count. */
void ui32_resize_to_fit(UI32_ARRAY *array);
/** @brief Append a uint32 value, growing storage if needed. */
void ui32_push(UI32_ARRAY *array, uint32_t element);
/** @brief Sort values in ascending order. */
void ui32_sort(UI32_ARRAY *array);
/** @brief Remove the last element if the array is non-empty. */
void ui32_pop(UI32_ARRAY *array);
/** @brief Compute SHA-256 checksum over active payload. */
void ui32_compute_hash(UI32_ARRAY *array);
/** @brief Verify the stored checksum against current payload. */
int ui32_verify_hash(UI32_ARRAY *array);
/** @brief Serialize count, payload, and checksum to a binary stream. */
int ui32_fwrite(UI32_ARRAY *array, FILE *file);
/** @brief Deserialize a UI32 array from a binary stream. */
UI32_ARRAY *ui32_fread(FILE *file);
/** @brief Execute UI32 test suite. */
int TEST_UI32_ARRAY(int verbose);
/** @} */

/** @name UI64 API */
/** @{ */
/** @brief Allocate a UI64 array with an initial capacity. */
UI64_ARRAY *ui64_init(int capacity);
/** @brief Free a UI64 array and null the caller pointer. */
void ui64_free(UI64_ARRAY **array);
/** @brief Resize UI64 storage to @p new_capacity (must be >= count). */
void ui64_resize_to(UI64_ARRAY *array, int new_capacity);
/** @brief Shrink UI64 storage so capacity equals count. */
void ui64_resize_to_fit(UI64_ARRAY *array);
/** @brief Append a uint64 value, growing storage if needed. */
void ui64_push(UI64_ARRAY *array, uint64_t element);
/** @brief Sort values in ascending order. */
void ui64_sort(UI64_ARRAY *array);
/** @brief Remove the last element if the array is non-empty. */
void ui64_pop(UI64_ARRAY *array);
/** @brief Compute SHA-256 checksum over active payload. */
void ui64_compute_hash(UI64_ARRAY *array);
/** @brief Verify the stored checksum against current payload. */
int ui64_verify_hash(UI64_ARRAY *array);
/** @brief Serialize count, payload, and checksum to a binary stream. */
int ui64_fwrite(UI64_ARRAY *array, FILE *file);
/** @brief Deserialize a UI64 array from a binary stream. */
UI64_ARRAY *ui64_fread(FILE *file);
/** @brief Execute UI64 test suite. */
int TEST_UI64_ARRAY(int verbose);
/** @} */

/**
 * @brief Run generic dispatch tests for C11 _Generic helper macros.
 * @param verbose Non-zero enables detailed logging.
 * @return 1 when all tests pass, otherwise 0.
 */
int TEST_GENERIC_INT_ARRAYS(int verbose);

/** @name Generic C11 Dispatch Macros
 *  @brief Type-based wrappers around ui16/ui32/ui64 APIs.
 *
 *  These are available only when compiling with C11 or newer.
 */
/** @{ */
#if __STDC_VERSION__ >= 201112L

/** @brief Dispatch to ui16_free/ui32_free/ui64_free. */
#define int_array_free(arr) _Generic((arr), \
    UI16_ARRAY * *: ui16_free,              \
    UI32_ARRAY * *: ui32_free,              \
    UI64_ARRAY * *: ui64_free)(arr)

/** @brief Dispatch to ui16_resize_to/ui32_resize_to/ui64_resize_to. */
#define int_array_resize_to(arr, cap) _Generic((arr), \
    UI16_ARRAY *: ui16_resize_to,                     \
    UI32_ARRAY *: ui32_resize_to,                     \
    UI64_ARRAY *: ui64_resize_to)(arr, cap)

/** @brief Dispatch to ui16_resize_to_fit/ui32_resize_to_fit/ui64_resize_to_fit. */
#define int_array_resize_to_fit(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_resize_to_fit,                \
    UI32_ARRAY *: ui32_resize_to_fit,                \
    UI64_ARRAY *: ui64_resize_to_fit)(arr)

/** @brief Dispatch to ui16_push/ui32_push/ui64_push. */
#define int_array_push(arr, val) _Generic((arr), \
    UI16_ARRAY *: ui16_push,                     \
    UI32_ARRAY *: ui32_push,                     \
    UI64_ARRAY *: ui64_push)(arr, val)

/** @brief Dispatch to ui16_sort/ui32_sort/ui64_sort. */
#define int_array_sort(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_sort,                \
    UI32_ARRAY *: ui32_sort,                \
    UI64_ARRAY *: ui64_sort)(arr)

/** @brief Dispatch to ui16_pop/ui32_pop/ui64_pop. */
#define int_array_pop(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_pop,                \
    UI32_ARRAY *: ui32_pop,                \
    UI64_ARRAY *: ui64_pop)(arr)

/** @brief Dispatch to ui16_compute_hash/ui32_compute_hash/ui64_compute_hash. */
#define int_array_compute_hash(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_compute_hash,                \
    UI32_ARRAY *: ui32_compute_hash,                \
    UI64_ARRAY *: ui64_compute_hash)(arr)

/** @brief Dispatch to ui16_verify_hash/ui32_verify_hash/ui64_verify_hash. */
#define int_array_verify_hash(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_verify_hash,                \
    UI32_ARRAY *: ui32_verify_hash,                \
    UI64_ARRAY *: ui64_verify_hash)(arr)

/** @brief Dispatch to ui16_fwrite/ui32_fwrite/ui64_fwrite. */
#define int_array_fwrite(arr, file) _Generic((arr), \
    UI16_ARRAY *: ui16_fwrite,                      \
    UI32_ARRAY *: ui32_fwrite,                      \
    UI64_ARRAY *: ui64_fwrite)(arr, file)

#endif // __STDC_VERSION__ >= 201112L
/** @} */

/** @} */

#endif // INT_ARRAYS_H
