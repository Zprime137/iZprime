
/**
 * @file bitmap.h
 * @brief Bitmap module for efficient bit array manipulation and prime sieve applications.
 *
 * @author iZprime.com
 * @date October 2025
 * @version 1.0
 *
 * ## Overview
 * This module provides a robust implementation of bit arrays (bitmaps) optimized for
 * prime sieve algorithms and other applications requiring efficient bit-level operations.
 * Each bitmap is represented by a BITMAP structure containing the bit array, its size,
 * and an optional SHA-256 hash for data integrity verification.
 *
 * ## Key Features
 * - Memory-efficient storage: 8 elements per byte
 * - Fast bitwise operations for individual and bulk bit manipulation
 * - Built-in SHA-256 hashing for data integrity
 * - File I/O support with automatic hash validation
 * - Thread-safe operations (when used with proper synchronization)
 * - Comprehensive bounds checking and error handling
 *
 * ## Memory Layout
 * Bits are stored in little-endian format within each byte:
 * - Bit 0 (LSB) is stored in bit position 0 of byte 0
 * - Bit 7 is stored in bit position 7 of byte 0
 * - Bit 8 is stored in bit position 0 of byte 1, etc.
 *
 * ## Usage Example
 * @code
 * // Create a bitmap with 1000 bits
 * BITMAP *bitmap = bitmap_init(1000);
 * if (bitmap == NULL) {
 *     // Handle allocation failure
 * }
 *
 * // Set some bits
 * bitmap_set_bit(bitmap, 42);
 * bitmap_set_bit(bitmap, 100);
 *
 * // Check a bit
 * int bit_value = bitmap_get_bit(bitmap, 42);  // Returns 1
 *
 * // Compute and validate hash
 * bitmap_compute_hash(bitmap);
 *
 * // Save to file
 * FILE *f = fopen("bitmap.bin", "wb");
 * bitmap_fwrite(bitmap, f);
 * fclose(f);
 *
 * // Clean up
 * bitmap_free(&bitmap);  // Sets bitmap to NULL
 * @endcode
 *
 * ## Function Categories
 *
 * ### Memory Management
 * - bitmap_init()        - Allocates and initializes a new bitmap
 * - bitmap_free()        - Deallocates bitmap and nullifies pointer
 * - bitmap_clone()       - Creates a deep copy of a bitmap
 *
 * ### Individual Bit Operations
 * - bitmap_set_bit()     - Sets a single bit to 1
 * - bitmap_clear_bit()   - Clears a single bit to 0
 * - bitmap_get_bit()     - Reads the value of a single bit
 * - bitmap_flip_bit()    - Toggles a single bit (0→1 or 1→0)
 *
 * ### Bulk Operations
 * - bitmap_set_all()     - Sets all bits to 1
 * - bitmap_clear_all()   - Clears all bits to 0
 * - bitmap_clear_steps() - Clears bits at regular intervals (for sieve algorithms)
 * - bitmap_clear_steps_simd() - SIMD-optimized version of clear_steps
 *
 * ### Data Integrity
 * - bitmap_compute_hash()   - Computes SHA-256 hash of bitmap data
 * - bitmap_validate_hash()  - Validates stored hash against current data
 *
 * ### File I/O
 * - bitmap_fwrite()      - Writes bitmap to file with automatic hash computation
 * - bitmap_fread()       - Reads bitmap from file with hash validation
 *
 * ### Testing
 * - TEST_BITMAP()        - Comprehensive test suite for all functions (see test/test_bitmap.c)
 *
 * @note All functions perform NULL pointer checking and bounds validation.
 * @note File I/O operations check for complete reads/writes.
 * @note The implementation is in src/modules/bitmap.c
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <utils.h>

/**
 * @struct BITMAP
 * @brief Core bitmap structure for efficient bit array storage and manipulation.
 *
 * This structure represents a dynamically-allocated bit array with integrated
 * data integrity checking via SHA-256 hashing. It is optimized for space efficiency,
 * storing 8 bits per byte.
 *
 * ## Structure Members
 * - **size**: Total number of bits in the bitmap (NOT bytes)
 * - **data**: Dynamically allocated byte array storing the bits
 * - **sha256**: SHA-256 hash for data integrity verification
 *
 * ## Memory Requirements
 * The data array requires ⌈size / 8⌉ bytes of memory.
 * Total structure size: sizeof(BITMAP) + ⌈size / 8⌉ bytes
 *
 * ## Bit Indexing
 * Bits are indexed from 0 to (size - 1). Within each byte, bits are stored
 * in little-endian format (bit 0 is the LSB).
 *
 * Example for size = 20:
 * - Byte 0: bits 0-7
 * - Byte 1: bits 8-15
 * - Byte 2: bits 16-19 (only lower 4 bits used)
 *
 * ## Hash Field
 * The sha256 field is:
 * - Initialized to all zeros by bitmap_init()
 * - Computed on-demand by bitmap_compute_hash()
 * - Automatically computed before file writes if all zeros
 * - Validated when reading from files
 *
 * @warning Do not directly modify the data or sha256 fields after hashing
 *          without recomputing the hash via bitmap_compute_hash().
 *
 * @note Always use bitmap_free() to deallocate; never use free() directly.
 */
typedef struct
{
    size_t size;                                ///< Total number of bits (NOT bytes) in the bitmap
    size_t byte_size;                           ///< Size of the data array in bytes (⌈size / 8⌉)
    unsigned char *data;                        ///< Dynamically allocated byte array storing bits
    unsigned char sha256[SHA256_DIGEST_LENGTH]; ///< SHA-256 hash for data integrity verification
} BITMAP;

// * MEMORY MANAGEMENT FUNCTIONS
BITMAP *bitmap_init(size_t size, int set_bits);
void bitmap_free(BITMAP **bitmap);
BITMAP *bitmap_clone(BITMAP *src);

// * BIT MANIPULATION FUNCTIONS
int bitmap_get_bit(BITMAP *bitmap, size_t idx);
void bitmap_set_bit(BITMAP *bitmap, size_t idx);
void bitmap_flip_bit(BITMAP *bitmap, size_t idx);
void bitmap_clear_bit(BITMAP *bitmap, size_t idx);

// * BULK OPERATIONS
void bitmap_set_all(BITMAP *bitmap);
void bitmap_clear_all(BITMAP *bitmap);
void bitmap_clear_steps(BITMAP *bitmap, uint64_t step, uint64_t start_idx, uint64_t limit);
void bitmap_clear_steps_simd(BITMAP *bitmap, uint64_t step, uint64_t start_idx, uint64_t limit);

// * DATA INTEGRITY (HASHING) FUNCTIONS
void bitmap_compute_hash(BITMAP *bitmap);
int bitmap_validate_hash(BITMAP *bitmap);

// * FILE I/O FUNCTIONS
int bitmap_fwrite(BITMAP *bitmap, FILE *file);
BITMAP *bitmap_fread(FILE *file);

// * TESTING FUNCTIONS
int TEST_BITMAP(int verbose);

#endif // BITMAP_H
