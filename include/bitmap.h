/**
 * @file bitmap.h
 * @brief Bitmap container and bit operations used by sieve implementations.
 *
 * The bitmap API is intentionally minimal and fast: a packed bit-array plus
 * helpers for per-bit operations, stepped clearing (for sieving), hashing, and
 * binary serialization.
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <utils.h>

/** @defgroup iz_bitmap Bitmap Module
 *  @brief Packed bit-array primitives for sieve and toolkit modules.
 *  @{ */

/**
 * @brief Packed bit-array with optional SHA-256 checksum.
 *
 * `size` is measured in bits and `byte_size` is the backing storage in bytes.
 * The checksum is maintained explicitly via bitmap_compute_hash() and verified
 * via bitmap_validate_hash().
 */
typedef struct
{
    size_t size;                                /**< Number of addressable bits. */
    size_t byte_size;                           /**< Number of bytes in @ref data. */
    unsigned char *data;                        /**< Packed bits (LSB-first per byte). */
    unsigned char sha256[SHA256_DIGEST_LENGTH]; /**< Cached SHA-256 checksum. */
} BITMAP;

/** @name Lifecycle */
/** @{ */
/**
 * @brief Allocate a bitmap with @p size bits.
 * @param size Number of bits to allocate (must be > 0).
 * @param set_bits Non-zero initializes all bits to 1, otherwise to 0.
 * @return Newly allocated bitmap, or NULL on allocation failure.
 */
BITMAP *bitmap_init(size_t size, int set_bits);

/**
 * @brief Free a bitmap and set the caller pointer to NULL.
 * @param bitmap Address of a BITMAP pointer.
 */
void bitmap_free(BITMAP **bitmap);

/**
 * @brief Deep-copy a bitmap, including data and checksum.
 * @param src Source bitmap.
 * @return Cloned bitmap, or NULL on failure.
 */
BITMAP *bitmap_clone(BITMAP *src);
/** @} */

/** @name Single-bit Operations */
/** @{ */
/**
 * @brief Read bit value at @p idx.
 * @param bitmap Bitmap to inspect.
 * @param idx Zero-based bit index.
 * @return 1 if set, otherwise 0.
 */
int bitmap_get_bit(BITMAP *bitmap, size_t idx);

/**
 * @brief Set bit at @p idx to 1.
 * @param bitmap Bitmap to modify.
 * @param idx Zero-based bit index.
 */
void bitmap_set_bit(BITMAP *bitmap, size_t idx);

/**
 * @brief Toggle bit at @p idx.
 * @param bitmap Bitmap to modify.
 * @param idx Zero-based bit index.
 */
void bitmap_flip_bit(BITMAP *bitmap, size_t idx);

/**
 * @brief Clear bit at @p idx (set to 0).
 * @param bitmap Bitmap to modify.
 * @param idx Zero-based bit index.
 */
void bitmap_clear_bit(BITMAP *bitmap, size_t idx);
/** @} */

/** @name Bulk Operations */
/** @{ */
/**
 * @brief Set all bits to 1.
 * @param bitmap Bitmap to modify.
 */
void bitmap_set_all(BITMAP *bitmap);

/**
 * @brief Clear all bits to 0.
 * @param bitmap Bitmap to modify.
 */
void bitmap_clear_all(BITMAP *bitmap);

/**
 * @brief Clear every @p step-th bit from @p start_idx up to @p limit.
 *
 * This is the core primitive for marking composite progressions in sieve code.
 *
 * @param bitmap Bitmap to modify.
 * @param step Index increment between cleared bits.
 * @param start_idx First index to clear.
 * @param limit Inclusive upper index bound.
 */
void bitmap_clear_steps(BITMAP *bitmap, uint64_t step, uint64_t start_idx, uint64_t limit);

/**
 * @brief SIMD-accelerated variant of bitmap_clear_steps().
 * @param bitmap Bitmap to modify.
 * @param step Index increment between cleared bits.
 * @param start_idx First index to clear.
 * @param limit Inclusive upper index bound.
 */
void bitmap_clear_steps_simd(BITMAP *bitmap, uint64_t step, uint64_t start_idx, uint64_t limit);
/** @} */

/** @name Integrity and I/O */
/** @{ */
/**
 * @brief Compute SHA-256 over bitmap data and store it in @ref BITMAP::sha256.
 * @param bitmap Bitmap to hash.
 */
void bitmap_compute_hash(BITMAP *bitmap);

/**
 * @brief Verify @ref BITMAP::sha256 against current bitmap data.
 * @param bitmap Bitmap to verify.
 * @return 1 if checksum matches, otherwise 0.
 */
int bitmap_validate_hash(BITMAP *bitmap);

/**
 * @brief Write bitmap payload and checksum to a binary stream.
 * @param bitmap Bitmap to serialize.
 * @param file Writable stream.
 * @return 1 on success, otherwise 0.
 */
int bitmap_fwrite(BITMAP *bitmap, FILE *file);

/**
 * @brief Read bitmap payload and checksum from a binary stream.
 * @param file Readable stream.
 * @return Newly allocated bitmap, or NULL on parse/verification failure.
 */
BITMAP *bitmap_fread(FILE *file);
/** @} */

/**
 * @brief Run bitmap module tests.
 * @param verbose Non-zero enables detailed logging.
 * @return 1 when all tests pass, otherwise 0.
 */
int TEST_BITMAP(int verbose);

/** @} */

#endif // BITMAP_H
