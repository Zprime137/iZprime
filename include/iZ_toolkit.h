#ifndef IZM_H
#define IZM_H

#include <utils.h>

// iZ utilities
uint64_t iZ(uint64_t x, int i);
void iZ_mpz(mpz_t z, mpz_t x, int i);

// =========================================================
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

// =========================================================
// * IZM structure and functions: Declarations
// =========================================================
/**
 * @brief iZm sieve assets for vx sieving.
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
uint64_t compute_vx_k(uint64_t n, int max_k);
uint64_t compute_l2_vx(uint64_t n);
void compute_max_vx(mpz_t vx, int bit_size);

// constructs a pre-sieved iZm base segment of size vx
void iZm_construct_vx_base(uint64_t vx, BITMAP *base_x5, BITMAP *base_x7);

// for horizontal sieving in iZm, solve for x, targets the first composite mark (x) of p in the vx segment of y
uint64_t iZm_solve_for_xp(int m_id, uint64_t p, uint64_t vx, uint64_t y);
uint64_t iZm_solve_for_xp_mpz(int m_id, uint64_t p, uint64_t vx, mpz_t y);
// for vertical sieving in iZm/vx, solve for y, targets the first composite mark (y) of p in the vy segment of x
int64_t iZm_solve_for_yp(int m_id, uint64_t p, uint64_t vx, uint64_t x);

// =========================================================
// * VX_SEG structure and functions: Declarations
// =========================================================
typedef struct
{
    int vx;             ///< The horizontal vector size.
    mpz_t y;            ///< mpz_t representation of y.
    mpz_t yvx;          ///< mpz_t representation of y * vx.
    mpz_t root_limit;   ///< mpz_t representation of sqrt(yvx + vx).
    int is_large_limit; ///< Flag indicating if probabilistic tests are needed.
    int mr_rounds;      ///< Number of Miller-Rabin rounds for primality testing.
    int start_x;        ///< First x value to start the main loop, default 1.
    int end_x;          ///< Upper bound for x in the main loop, default vx.
    BITMAP *x5;         ///< Bitmap for iZm5 segment.
    BITMAP *x7;         ///< Bitmap for iZm7 segment.
    int p_count;        ///< Number of primes found.
    UI16_ARRAY *p_gaps; ///< Pointer to the p_gaps array.
    int bit_ops;        ///< Number of bitwise mark operations performed.
    int p_test_ops;     ///< Number of primality test operations performed.
} VX_SEG;

VX_SEG *vx_init(IZM *iZm, int start_x, int end_x, char *y_str, int mr_rounds);
void vx_free(VX_SEG **vx_obj);

void vx_collect_p_gaps(VX_SEG *vx_obj);
void vx_full_sieve(VX_SEG *vx_obj, int collect_p_gaps);
void vx_stream_file(VX_SEG *vx_obj, FILE *output);

// =========================================================
int vx_search_prime(mpz_t p, int m_id, int vx, int bit_size); // horizontal random prime search
int vy_search_prime(mpz_t p, int m_id, mpz_t vx);             // vertical random prime search

// =========================================================
// * Test functions declarations
// =========================================================
int TEST_IZM(int verbose);
int TEST_VX_SEG(int verbose);

#endif // IZM_H
