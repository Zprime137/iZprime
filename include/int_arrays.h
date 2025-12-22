/**
 * @file int_arrays.h
 * @brief Dynamic integer array module for efficient storage and manipulation of integer collections.
 *
 * @author iZprime.com
 * @date October 2025
 * @version 1.0
 *
 * ## Overview
 * This module provides robust implementations of dynamic integer arrays for both 16-bit
 * and 64-bit unsigned integers. Each array type (UI16_ARRAY and UI64_ARRAY) features
 * automatic resizing, data integrity verification via SHA-256 hashing, and binary file I/O
 * support with automatic hash validation.
 *
 * ## Key Features
 * - Dynamic capacity management with automatic 2x growth on overflow
 * - Built-in SHA-256 hashing for data integrity verification
 * - File I/O support with automatic hash validation
 * - Efficient append operations with amortized O(1) complexity
 * - Comprehensive error handling and NULL pointer checking
 * - Memory-efficient storage with minimal overhead
 *
 * ## Usage Example
 * @code
 * // Create a 16-bit integer array
 * UI16_ARRAY *arr = ui16_init(100);  // Initial capacity: 100
 * if (arr == NULL) {
 *     // Handle allocation failure
 * }
 *
 * // Append elements (auto-resizes when needed)
 * for (int i = 0; i < 150; i++) {
 *     ui16_push(arr, (uint16_t)i);
 * }
 *
 * // Compute and validate hash
 * ui16_compute_hash(arr);
 * if (ui16_verify_hash(arr)) {
 *     printf("Hash valid!\n");
 * }
 *
 * // Save to file
 * FILE *f = fopen("data.bin", "wb");
 * ui16_fwrite(arr, f);
 * fclose(f);
 *
 * // Clean up
 * ui16_free(&arr);  // Sets arr to NULL
 * @endcode
 *
 * ## Function Categories
 *
 * ### UI16_ARRAY Functions (16-bit unsigned integers)
 * - ui16_init()          - Allocates and initializes a new array
 * - ui16_free()          - Deallocates array and nullifies pointer
 * - ui16_push()          - Appends element with automatic resize
 * - ui16_resize_to_fit() - Resizes array to exact fit (count == capacity)
 * - ui16_compute_hash()  - Computes SHA-256 hash of array data
 * - ui16_verify_hash()   - Validates stored hash against current data
 * - ui16_fwrite()        - Writes array to file with hash
 * - ui16_fread()         - Reads array from file with hash validation
 * - TEST_UI16_ARRAY()    - Comprehensive test suite
 *
 * ### UI64_ARRAY Functions (64-bit unsigned integers)
 * - ui64_init()          - Allocates and initializes a new array
 * - ui64_free()          - Deallocates array and nullifies pointer
 * - ui64_push()          - Appends element with automatic resize
 * - ui64_pop()           - Removes last element from array
 * - ui64_resize_to_fit() - Resizes array to exact fit (count == capacity)
 * - ui64_compute_hash()  - Computes SHA-256 hash of array data
 * - ui64_verify_hash()   - Validates stored hash against current data
 * - ui64_fwrite()        - Writes array to file with hash
 * - ui64_fread()         - Reads array from file with hash validation
 * - TEST_UI64_ARRAY()    - Comprehensive test suite
 *
 * ## Performance Characteristics
 * - Initialization: O(1)
 * - Append (amortized): O(1)
 * - Append (worst case): O(n) when resize occurs
 * - Hash computation: O(n)
 * - Hash verification: O(n)
 * - File I/O: O(n)
 * - Memory overhead: ~24 bytes + SHA256_DIGEST_LENGTH per array
 *
 * ## Memory Management
 * - Arrays grow by 1000 capacity when full
 * - Use ui16_resize_to_fit()/ui64_resize_to_fit() to shrink to exact fit
 * - Always use ui16_free()/ui64_free() to deallocate - never free() directly
 * - Free operations nullify pointers to prevent use-after-free bugs
 *
 * @note All functions perform NULL pointer checking and bounds validation.
 * @note File I/O operations automatically compute and validate SHA-256 hashes.
 * @note The implementation is in src/modules/int_arrays.c
 */

#ifndef INT_ARRAYS_H
#define INT_ARRAYS_H

#include <utils.h>

/*
 * ========================================================================
 * UI16_ARRAY - 16-BIT UNSIGNED INTEGER ARRAY
 * ========================================================================
 */
typedef struct
{
    int capacity;                               ///< Total capacity (max count of elements) of the array.
    int count;                                  ///< Number of elements currently stored.
    uint16_t *array;                            ///< Pointer to the dynamically allocated uint16_t array.
    unsigned char sha256[SHA256_DIGEST_LENGTH]; ///< SHA-256 hash of array for validation.
} UI16_ARRAY;

UI16_ARRAY *ui16_init(int capacity);
void ui16_free(UI16_ARRAY **array);
void ui16_resize_to(UI16_ARRAY *array, int new_capacity);
void ui16_resize_to_fit(UI16_ARRAY *array);
void ui16_push(UI16_ARRAY *array, uint16_t element);
void ui16_pop(UI16_ARRAY *array);
void ui16_compute_hash(UI16_ARRAY *array);
int ui16_verify_hash(UI16_ARRAY *array);
int ui16_fwrite(UI16_ARRAY *array, FILE *file);
UI16_ARRAY *ui16_fread(FILE *file);
int TEST_UI16_ARRAY(int verbose);

/** ========================================================================
 * * UI32_ARRAY - 32-BIT UNSIGNED INTEGER ARRAY
 * ========================================================================
 */
typedef struct
{
    int capacity;                               ///< Total capacity (max count of elements) of the array.
    int count;                                  ///< Number of elements currently stored.
    uint32_t *array;                            ///< Pointer to the dynamically allocated uint32_t array.
    unsigned char sha256[SHA256_DIGEST_LENGTH]; ///< SHA-256 hash of array for validation.
} UI32_ARRAY;

UI32_ARRAY *ui32_init(int capacity);
void ui32_free(UI32_ARRAY **array);
void ui32_resize_to(UI32_ARRAY *array, int new_capacity);
void ui32_resize_to_fit(UI32_ARRAY *array);
void ui32_push(UI32_ARRAY *array, uint32_t element);
void ui32_pop(UI32_ARRAY *array);
void ui32_compute_hash(UI32_ARRAY *array);
int ui32_verify_hash(UI32_ARRAY *array);
int ui32_fwrite(UI32_ARRAY *array, FILE *file);
UI32_ARRAY *ui32_fread(FILE *file);
int TEST_UI32_ARRAY(int verbose);

/** ========================================================================
 * * UI64_ARRAY - 64-BIT UNSIGNED INTEGER ARRAY
 * ========================================================================
 */
typedef struct
{
    int capacity;                               ///< Total capacity (max count of elements) of the array.
    int count;                                  ///< Number of elements currently stored.
    uint64_t *array;                            ///< Pointer to the dynamically allocated uint64_t array.
    unsigned char sha256[SHA256_DIGEST_LENGTH]; ///< SHA-256 hash of array for validation.
} UI64_ARRAY;

UI64_ARRAY *ui64_init(int capacity);
void ui64_free(UI64_ARRAY **array);
void ui64_resize_to(UI64_ARRAY *array, int new_capacity);
void ui64_resize_to_fit(UI64_ARRAY *array);
void ui64_push(UI64_ARRAY *array, uint64_t element);
void ui64_pop(UI64_ARRAY *array);
void ui64_compute_hash(UI64_ARRAY *array);
int ui64_verify_hash(UI64_ARRAY *array);
int ui64_fwrite(UI64_ARRAY *array, FILE *file);
UI64_ARRAY *ui64_fread(FILE *file);
int TEST_UI64_ARRAY(int verbose);

/**
 * @brief Runs generic interface tests (C11 _Generic).
 *
 * Tests the generic macros int_array_push, int_array_pop, etc.
 *
 * @param verbose If non-zero, prints detailed test output
 * @return int 1 if all tests pass, 0 if any test fails
 */
int TEST_GENERIC_INT_ARRAYS(int verbose);

/*
 * ========================================================================
 * GENERIC INTERFACE (C11 _Generic)
 * ========================================================================
 */

#if __STDC_VERSION__ >= 201112L

#define int_array_free(arr) _Generic((arr), \
    UI16_ARRAY * *: ui16_free,              \
    UI32_ARRAY * *: ui32_free,              \
    UI64_ARRAY * *: ui64_free)(arr)

#define int_array_resize_to(arr, cap) _Generic((arr), \
    UI16_ARRAY *: ui16_resize_to,                     \
    UI32_ARRAY *: ui32_resize_to,                     \
    UI64_ARRAY *: ui64_resize_to)(arr, cap)

#define int_array_resize_to_fit(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_resize_to_fit,                \
    UI32_ARRAY *: ui32_resize_to_fit,                \
    UI64_ARRAY *: ui64_resize_to_fit)(arr)

#define int_array_push(arr, val) _Generic((arr), \
    UI16_ARRAY *: ui16_push,                     \
    UI32_ARRAY *: ui32_push,                     \
    UI64_ARRAY *: ui64_push)(arr, val)

#define int_array_pop(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_pop,                \
    UI32_ARRAY *: ui32_pop,                \
    UI64_ARRAY *: ui64_pop)(arr)

#define int_array_compute_hash(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_compute_hash,                \
    UI32_ARRAY *: ui32_compute_hash,                \
    UI64_ARRAY *: ui64_compute_hash)(arr)

#define int_array_verify_hash(arr) _Generic((arr), \
    UI16_ARRAY *: ui16_verify_hash,                \
    UI32_ARRAY *: ui32_verify_hash,                \
    UI64_ARRAY *: ui64_verify_hash)(arr)

#define int_array_fwrite(arr, file) _Generic((arr), \
    UI16_ARRAY *: ui16_fwrite,                      \
    UI32_ARRAY *: ui32_fwrite,                      \
    UI64_ARRAY *: ui64_fwrite)(arr, file)

// Note: init and fread cannot be generic based on arguments easily
// because init takes an int (same for both) and fread takes a FILE* (same for both).
// Users must use specific functions for creation: ui16_init / ui32_init / ui64_init.

#endif // __STDC_VERSION__ >= 201112L

#endif // INT_ARRAYS_H
