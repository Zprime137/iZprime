#ifndef IZM_H
#define IZM_H

#include <utils.h>

// * iZm structure and functions: Declarations
// =========================================================

// iZm standard VX_k sizes (product of first k primes > 3)
#define VX2 (5 * 7ULL)    // 35
#define VX3 (VX2 * 11ULL) // 385
#define VX4 (VX3 * 13ULL) // 5005
#define VX5 (VX4 * 17ULL) // 85,085
#define VX6 (VX5 * 19ULL) // 1,616,615
#define VX7 (VX6 * 23ULL) // 37,260,615
#define VX8 (VX7 * 29ULL) // 1,080,558,835

// Default Miller-Rabin primality test rounds, balances between speed and accuracy
#define MR_ROUNDS 25

/**
 * @brief iZm sieve assets for vx prime sieve.
 *
 * This structure contains the size of the segment (vx), a pointer to the root primes
 * used for sieving, and two base bitmaps (base_x5 and base_x7) pre-sieved for
 * primes that divide vx.
 *
 * @param vx (int) The size of the segment.
 * @param root_primes (UI64_ARRAY *) Pointer to the root primes (< vx) used for sieving.
 * @param base_x5 (BITMAP *) Pointer to the base bitmap for iZm5/vx.
 * @param base_x7 (BITMAP *) Pointer to the base bitmap for iZm7/vx.
 */
typedef struct
{
    int vx;                  ///< Size of the segment
    int k_vx;                ///< Number of primes multiplied to form vx
    BITMAP *base_x5;         ///< Base bitmap for iZm5/vx segment
    BITMAP *base_x7;         ///< Base bitmap for iZm7/vx segment
    UI64_ARRAY *root_primes; ///< Root primes < vx used for sieving
} IZM;

// iZm structure init, clone, and free functions
IZM *iZm_init(size_t vx);
IZM *iZm_clone(IZM *src);
void iZm_free(IZM **iZm);

// compute vx functions
uint64_t iZm_compute_limited_vx(uint64_t n, int max_k);
void iZm_compute_max_vx(mpz_t vx, int bit_size);

// this constructs a pre-sieved iZm base segment of size vx
void iZm_construct_vx_base(uint64_t vx, BITMAP *base_x5, BITMAP *base_x7);

// for horizontal sieving in iZm, solve for x, targets the first composite mark (x) of p in the vx segment of y
uint64_t iZm_solve_for_xp(int m_id, uint64_t p, uint64_t vx, uint64_t y);
uint64_t iZm_solve_for_xp_mpz(int m_id, uint64_t p, uint64_t vx, mpz_t y);
// for vertical sieving in iZm/vx, solve for y, targets the first composite mark (y) of p in the vy segment of x
int64_t iZm_solve_for_yp(int m_id, uint64_t p, uint64_t vx, uint64_t x);

int TEST_IZM(int verbose);

#endif // IZM_H
