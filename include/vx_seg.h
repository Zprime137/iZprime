
/**
 * @file vx_seg.h
 * @brief Header file for VX_SEG and VX_RANGE structures and their associated functions.
 *
 * @description:
 * This file contains the definition of two data structures and their associated functions.
 *
 * @api:
 * @struct VX_SEG:
 * * ** VX_SEG related functions:
 * - @vx_init: Initializes a new VX_SEG structure with the given y string.
 * - @vx_free: Frees the memory allocated for the VX_SEG structure.
 * - @vx_det_sieve: Initializes and sieves the x5 and x7 bitmaps for root primes in iZm.
 * - @vx_prob_sieve: Performs probabilistic sieving on the VX_SEG structure.
 * - @vx_full_sieve: Performs the full sieve process on the VX_SEG structure, collecting prime gaps if specified.
 * - @vx_collect_p_gaps: Initializes and collects the prime gaps for the VX_SEG structure.
 * - @vx_nth_p: Retrieves the nth prime from the VX_SEG structure.
 * - @vx_write_file: Writes the contents of the VX_SEG structure to a file.
 * - @vx_read_file: Reads the contents of a file into a VX_SEG structure.
 *
 */

#ifndef VX_OBJ_H
#define VX_OBJ_H

#include <utils.h>
#include <iZm.h>

#define VX_EXT ".vx" // File extension for VX object files

/**
 * @struct VX_SEG
 * @brief Structure representing a vx segment and its metadata.
 *
 * @param vx The vector size vx.
 * @param y A pointer to the numeric string y.
 * @param yvx mpz_t representation of y * vx.
 * @param root_limit mpz_t representation of the root limit for sieving.
 * @param is_large_limit Flag indicating if probabilistic tests are needed.
 * @param mr_rounds Number of Miller-Rabin rounds for probabilistic primality testing.
 * @param start_x First x value to start the main loop, default 1.
 * @param end_x Upper bound for indices x in the main loop, default vx.
 * @param x5 Bitmap for iZm5/vx segment.
 * @param x7 Bitmap for iZm7/vx segment.
 * @param p_count The number of primes found.
 * @param p_gaps A pointer to the UI16_ARRAY of prime gaps.
 * @param bit_ops The number of bitwise mark operations performed during the sieve process.
 * @param p_test_ops The number of primality test operations performed during the sieve process.
 */
typedef struct
{
    int vx;             ///< The horizontal vector size.
    mpz_t y;            ///< mpz_t representation of the numeric y string.
    mpz_t yvx;          ///< mpz_t representation of y * vx.
    mpz_t root_limit;   ///< mpz_t representation of the root limit for sieving.
    int is_large_limit; ///< Flag indicating if probabilistic tests are needed.
    int mr_rounds;      ///< Number of Miller-Rabin rounds for primality testing.
    int start_x;        ///< First x value to start the main loop, default 1.
    int end_x;          ///< Upper bound for indices x in the main loop, default vx.
    BITMAP *x5;         ///< Bitmap for iZm5/vx segment.
    BITMAP *x7;         ///< Bitmap for iZm7/vx segment.
    int p_count;        ///< Number of elements in the p_gaps array.
    UI16_ARRAY *p_gaps; ///< Pointer to the p_gaps array.
    int bit_ops;        ///< Number of bitwise mark operations performed.
    int p_test_ops;     ///< Number of primality test operations performed.
} VX_SEG;

VX_SEG *vx_init(int vx, char *y, int mr_rounds);
void vx_free(VX_SEG **vx_obj);

void vx_det_sieve(IZM *iZm, VX_SEG *vx_obj);
void vx_full_sieve(IZM *iZm, VX_SEG *vx_obj, int collect_p_gaps);

void vx_collect_p_gaps(VX_SEG *vx_obj);
int vx_nth_p(mpz_t p, VX_SEG *vx_obj, int n);

int vx_write_file(VX_SEG *vx_obj, char *filename);
VX_SEG *vx_read_file(char *filename);

int TEST_VX_SEG(int verbose);

#endif // VX_OBJ_H
