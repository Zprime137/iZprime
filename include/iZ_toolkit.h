/**
 * @file iZ_toolkit.h
 * @brief Internal toolkit for iZ index space and iZm/VX segmented sieving.
 *
 * The toolkit layer provides the core data structures and primitives used by
 * the SiZ/SiZm algorithms:
 * - iZ index mapping for integers of the form $6x\pm1$.
 * - iZm base segment construction (VX primorial segment sizes).
 * - VX segment objects that encapsulate bitmaps, counters, and prime-gap output.
 */

#ifndef IZ_TOOLKIT_H
#define IZ_TOOLKIT_H

#include <utils.h>

/** @defgroup iz_toolkit Toolkit (iZ/iZm)
 *  @brief Core building blocks for iZ-space sieving.
 *  @{
 */

// iZ utilities
/**
 * @brief Map an iZ coordinate to its integer value: $iZ(x,i)=6x+i$.
 * @param x iZ coordinate (x >= 0).
 * @param i Either -1 or +1 in normal iZ usage.
 * @return The mapped integer value.
 */
uint64_t iZ(uint64_t x, int i);

/**
 * @brief GMP version of iZ(): compute $z=6x+i$.
 * @param z Output value.
 * @param x Input x coordinate.
 * @param i Either -1 or +1 in normal iZ usage.
 */
void iZ_mpz(mpz_t z, mpz_t x, int i);

// sieve utilities
/**
 * @brief Process iZ bitmaps and append surviving primes to @p primes.
 *
 * The two bitmaps represent the iZ- and iZ+ lines (6x-1 and 6x+1).
 *
 * @param primes Output array of primes.
 * @param x5 Bitmap for candidates on 6x-1.
 * @param x7 Bitmap for candidates on 6x+1.
 * @param x_limit Upper x bound (exclusive).
 */
void process_iZ_bitmaps(UI64_ARRAY *primes, BITMAP *x5, BITMAP *x7, uint64_t x_limit);

/**
 * @brief Populate @p primes with primes up to @p limit.
 *
 * Used to obtain root primes for deterministic sieving.
 */
void get_root_primes(UI64_ARRAY *primes, uint64_t limit);

// iZm standard VX_k sizes (product of first k primes > 3)
/** VX segment size for k=2 (35). */
#define VX2 (5 * 7ULL)
/** VX segment size for k=3 (385). */
#define VX3 (VX2 * 11ULL)
/** VX segment size for k=4 (5005). */
#define VX4 (VX3 * 13ULL)
/** VX segment size for k=5 (85085). */
#define VX5 (VX4 * 17ULL)
/** VX segment size for k=6 (1616615). */
#define VX6 (VX5 * 19ULL)
/** VX segment size for k=7 (37260615). */
#define VX7 (VX6 * 23ULL)
/** VX segment size for k=8 (1080558835). */
#define VX8 (VX7 * 29ULL)

/** Default Miller–Rabin primality test rounds (speed/accuracy tradeoff). */
#define MR_ROUNDS 25

// =========================================================
// * IZM structure and functions: Declarations
// =========================================================
/**
 * @brief iZm precomputed assets for VX-segment sieving.
 *
 * An `IZM` instance owns:
 * - `root_primes`: primes used to mark composites in each segment.
 * - `base_x5`/`base_x7`: pre-sieved templates for 6x-1 and 6x+1 lines.
 *
 * The object can be cloned for multi-process usage where each worker needs
 * independent bitmap state.
 */
typedef struct
{
    int vx;                  ///< Size of the segment
    int k_vx;                ///< Number of primes multiplied to form vx
    BITMAP *base_x5;         ///< Base bitmap for iZm5/vx segment
    BITMAP *base_x7;         ///< Base bitmap for iZm7/vx segment
    UI64_ARRAY *root_primes; ///< Root primes < vx used for sieving
} IZM;

/** @name IZM lifecycle */
///@{
IZM *iZm_init(size_t vx);
IZM *iZm_clone(IZM *src);
void iZm_free(IZM **iZm);
///@}

/** @name VX sizing helpers */
///@{
uint64_t compute_vx_k(uint64_t n, int max_k);
uint64_t compute_l2_vx(uint64_t n);
void compute_max_vx(mpz_t vx, int bit_size);
///@}

/**
 * @brief Construct a pre-sieved base segment for a VX value.
 * @param vx Segment size.
 * @param base_x5 Output base bitmap for 6x-1 candidates.
 * @param base_x7 Output base bitmap for 6x+1 candidates.
 */
void iZm_construct_vx_base(uint64_t vx, BITMAP *base_x5, BITMAP *base_x7);

/** @name iZm modular solvers
 *  @brief Locate the first composite hit of a prime within a segment.
 */
///@{
/**
 * @brief Solve for the first x-hit of prime @p p in a VX segment at y.
 */
uint64_t iZm_solve_for_xp(int m_id, uint64_t p, uint64_t vx, uint64_t y);
uint64_t iZm_solve_for_xp_mpz(int m_id, uint64_t p, uint64_t vx, mpz_t y);
/**
 * @brief Solve for the first y-hit of prime @p p in a VY segment at x.
 */
int64_t iZm_solve_for_yp(int m_id, uint64_t p, uint64_t vx, uint64_t x);
///@}

// =========================================================
// * VX_SEG structure and functions: Declarations
// =========================================================
/**
 * @brief Represents a single VX-sized sieving segment at a specific y coordinate.
 *
 * VX segments are the work units used by SiZm and range scanning.
 */
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

/** @} */

#endif // IZ_TOOLKIT_H
