/**
 * @file iZ_toolkit.h
 * @brief Core iZ/iZm primitives used by SiZ-family algorithms.
 *
 * This layer exposes index-space mapping helpers, VX sizing utilities,
 * pre-sieved iZm base construction, per-segment sieve objects, and internal
 * prime-search routines used by high-level APIs in iZ_api.h.
 */

#ifndef IZ_TOOLKIT_H
#define IZ_TOOLKIT_H

#include <utils.h>      // Common utilities, types, and dependencies.
#include <int_arrays.h> // Integer array containers.
#include <bitmap.h>     // Packed bit-array utilities.

/** Forward declaration; concrete definition lives in iZ_api.h. */
typedef struct INPUT_SIEVE_RANGE INPUT_SIEVE_RANGE;

/** @defgroup iz_toolkit Toolkit (iZ/iZm)
 *  @brief Internal building blocks for SiZ and SiZm implementations.
 *  @{ */

/** @name iZ Mapping Helpers */
/** @{ */
/**
 * @brief Map iZ coordinates to an integer: `6*x + i`.
 * @param x iZ x-coordinate.
 * @param i Matrix identifier, typically -1 or +1.
 * @return Mapped integer value.
 */
uint64_t iZ(uint64_t x, int i);

/**
 * @brief GMP variant of iZ().
 * @param z Output value.
 * @param x Input x-coordinate.
 * @param i Matrix identifier, typically -1 or +1.
 */
void iZ_mpz(mpz_t z, mpz_t x, int i);

/**
 * @brief Traverse iZ bitmaps, emit surviving primes, and mark composites.
 * @param primes Output prime array.
 * @param x5 Bitmap for 6x-1 candidates.
 * @param x7 Bitmap for 6x+1 candidates.
 * @param x_limit Exclusive x upper bound.
 */
void process_iZ_bitmaps(UI64_ARRAY *primes, BITMAP *x5, BITMAP *x7, uint64_t x_limit);

/**
 * @brief Generate primes up to @p limit for deterministic sieving.
 * @param primes Output array, appended in ascending order.
 * @param limit Numeric upper bound.
 */
void get_root_primes(UI64_ARRAY *primes, uint64_t limit);

/**
 * @brief Check the primality of a number using GMP's probabilistic test.
 * @param n Number to check.
 * @param rounds Number of Miller-Rabin rounds.
 * @return Non-zero if probably prime, 0 if composite.
 */
int check_primality(mpz_t n, int rounds);
/** @} */

/** @name Standard VX Sizes (primorial products excluding 2,3) */
/** @{ */
#define VX2 (5 * 7ULL)    /**< 35 */
#define VX3 (VX2 * 11ULL) /**< 385 */
#define VX4 (VX3 * 13ULL) /**< 5005 */
#define VX5 (VX4 * 17ULL) /**< 85085 */
#define VX6 (VX5 * 19ULL) /**< 1616615 */
#define VX7 (VX6 * 23ULL) /**< 37260615 */
#define VX8 (VX7 * 29ULL) /**< 1080558835 */
/** @} */

/** Default Miller-Rabin rounds used by toolkit search/sieve helpers. */
#define MR_ROUNDS 25

/**
 * @brief Precomputed iZm assets for repeated VX-segment sieving.
 */
typedef struct
{
    int vx;                  /**< Segment width in iZ x-units. */
    int k_vx;                /**< Count of small primes dividing vx (excluding 2,3). */
    BITMAP *base_x5;         /**< Pre-sieved base bitmap for 6x-1 line. */
    BITMAP *base_x7;         /**< Pre-sieved base bitmap for 6x+1 line. */
    UI64_ARRAY *root_primes; /**< Root primes used for deterministic marking. */
} IZM;

/** @name IZM Lifecycle */
/** @{ */
/**
 * @brief Allocate and initialize an IZM object for a given VX.
 * @param vx Segment width; must be odd, not divisible by 3, and >= 35.
 * @return Initialized IZM object, or NULL on failure.
 */
IZM *iZm_init(size_t vx);

/**
 * @brief Deep-copy an IZM object for per-worker ownership.
 * @param src Source IZM instance.
 * @return Newly allocated clone, or NULL on failure.
 */
IZM *iZm_clone(IZM *src);

/**
 * @brief Release an IZM object and set the caller pointer to NULL.
 * @param iZm Address of an IZM pointer.
 */
void iZm_free(IZM **iZm);
/** @} */

/** @name VX Selection Helpers */
/** @{ */
/**
 * @brief Calculate VX_{ @p k }.
 * @param k Number of multiplicative prime factors (>3).
 * @return Selected VX size.
 */
uint64_t compute_vx_k(int k);

/**
 * @brief Choose VX using an L2-cache-aware heuristic.
 * @param n Target numeric limit.
 * @return Selected VX size.
 */
uint64_t compute_l2_vx(uint64_t n);

/**
 * @brief Compute largest VX below 2^bit_size.
 * @param vx Output mpz_t containing VX.
 * @param bit_size Bit-size ceiling.
 */
void compute_max_vx(mpz_t vx, int bit_size);
/** @} */

/**
 * @brief Build pre-sieved base bitmaps for a VX segment.
 * @param vx Segment width.
 * @param base_x5 Output bitmap for 6x-1 candidates.
 * @param base_x7 Output bitmap for 6x+1 candidates.
 */
void iZm_construct_vx_base(uint64_t vx, BITMAP *base_x5, BITMAP *base_x7);

/** @name Modular Hit Solvers */
/** @{ */
/**
 * @brief Solve first x-hit of prime @p p for line @p m_id in segment y.
 * @param m_id Line id (-1 for x5, +1 for x7).
 * @param p Prime used for marking.
 * @param vx Segment width.
 * @param y Segment index.
 * @return First x index to clear for this prime/line.
 */
uint64_t iZm_solve_for_x0(int m_id, uint64_t p, uint64_t vx, uint64_t y);

/**
 * @brief GMP variant of iZm_solve_for_x0() for very large y.
 * @param m_id Line id (-1 for x5, +1 for x7).
 * @param p Prime used for marking.
 * @param vx Segment width.
 * @param y Segment index as mpz.
 * @return First x index to clear for this prime/line.
 */
uint64_t iZm_solve_for_x0_mpz(int m_id, uint64_t p, uint64_t vx, mpz_t y);

/**
 * @brief Solve first y-hit for fixed x in vertical (vy) scanning.
 * @param m_id Line id (-1 for x5, +1 for x7).
 * @param p Prime used for marking.
 * @param vx Segment width.
 * @param x Fixed x-coordinate.
 * @return y index on success, -1 when no modular solution exists.
 */
int64_t iZm_solve_for_y0(int m_id, uint64_t p, uint64_t vx, uint64_t x);
/** @} */

/**
 * @brief Runtime state for one VX segment at a specific y.
 */
typedef struct
{
    int vx;             /**< Segment width. */
    mpz_t y;            /**< Segment index y. */
    mpz_t yvx;          /**< Cached product y*vx. */
    mpz_t root_limit;   /**< sqrt(iZ(yvx + vx, +1)). */
    int is_large_limit; /**< Non-zero => requires probabilistic primality checks. */
    int mr_rounds;      /**< Miller-Rabin rounds when probabilistic checks are used. */
    int start_x;        /**< Inclusive start x for this segment. */
    int end_x;          /**< Inclusive end x for this segment. */
    BITMAP *x5;         /**< Candidate bitmap for 6x-1. */
    BITMAP *x7;         /**< Candidate bitmap for 6x+1. */
    int p_count;        /**< Count of primes found in this segment. */
    UI16_ARRAY *p_gaps; /**< Optional prime-gap encoding for streamed output. */
    int bit_ops;        /**< Approximate deterministic mark operations. */
    int p_test_ops;     /**< Probabilistic primality tests executed. */
} VX_SEG;

/** @name VX Segment Lifecycle and Execution */
/** @{ */
/**
 * @brief Initialize and deterministically sieve one VX segment.
 * @param iZm Initialized toolkit context.
 * @param start_x Inclusive x start index.
 * @param end_x Inclusive x end index.
 * @param y_str Segment index y as a decimal string.
 * @param mr_rounds Miller-Rabin rounds (0 uses default).
 * @return Initialized and partially processed segment, or NULL on failure.
 */
VX_SEG *vx_init(IZM *iZm, int start_x, int end_x, char *y_str, int mr_rounds);

/**
 * @brief Free a VX segment and all owned resources.
 * @param vx_obj Address of a VX_SEG pointer.
 */
void vx_free(VX_SEG **vx_obj);

/**
 * @brief Extract prime-gap encoding from a fully sieved segment.
 * @param vx_obj Segment object.
 */
void vx_collect_p_gaps(VX_SEG *vx_obj);

/**
 * @brief Complete segment processing (probabilistic stage and optional gaps).
 * @param vx_obj Segment object.
 * @param collect_p_gaps Non-zero to populate @ref VX_SEG::p_gaps.
 */
void vx_full_sieve(VX_SEG *vx_obj, int collect_p_gaps);

/**
 * @brief Stream segment primes to an output stream.
 * @param vx_obj Segment object.
 * @param output Writable output stream (e.g. stdout or a file).
 */
void vx_stream(VX_SEG *vx_obj, FILE *output);
/** @} */

/**
 * @brief Precomputed iZm coordinates for an inclusive numeric interval.
 *
 * This structure maps an input range `[Zs, Ze]` into the iZ index space used
 * by segmented SiZ routines:
 * - `X*` are integer-domain indices such that `Z ~= 6X +/- 1`.
 * - `Y*` are segment indices (`X / vx`).
 * - `y_range = Ye - Ys` and is used to drive segment iteration counts.
 *
 * A negative `y_range` marks an invalid/unusable mapping (bad input or range
 * outside current implementation bounds).
 */
typedef struct
{
    int vx;      /**< iZm segment width. */
    mpz_t Zs;    /**< Inclusive lower bound of the search interval. */
    mpz_t Ze;    /**< Inclusive upper bound of the search interval. */
    mpz_t Xs;    /**< x_Zs = Zs / 6. */
    mpz_t Xe;    /**< x_Ze = Ze / 6. */
    mpz_t Ys;    /**< y_Zs = Xs / vx. */
    mpz_t Ye;    /**< y_Ze = Xe / vx. */
    int y_range; /**< Number of y-steps minus one (`Ye - Ys`), or -1 when invalid. */
} IZM_RANGE_INFO;

/**
 * @brief Map a decimal range input into iZm coordinates and segment bounds.
 * @param input_range Range input (`start`, `range`).
 * @param vx iZm segment width.
 * @return Initialized range-info object; `y_range < 0` indicates invalid input
 *         or an unsupported y-span for the current implementation.
 */
IZM_RANGE_INFO range_info_init(INPUT_SIEVE_RANGE *input_range, int vx);
/** @brief Clear all GMP fields owned by @p info. */
void range_info_free(IZM_RANGE_INFO *info);

/** @name Random Prime Search Routines */
/** @{ */
/**
 * @brief Horizontal iZm/VX random-prime search.
 * @param p Output prime.
 * @param m_id Requested line id (-1, +1, or random when other value).
 * @param vx Segment width.
 * @param bit_size Target bit size.
 * @return 1 on success, otherwise 0.
 */
int vx_search_prime(mpz_t p, int m_id, int vx, int bit_size);

/**
 * @brief Vertical iZm/VY random-prime search.
 * @param p Output prime.
 * @param m_id Requested line id (-1, +1, or random when other value).
 * @param vx Segment width.
 * @return 1 on success, otherwise 0.
 */
int vy_search_prime(mpz_t p, int m_id, mpz_t vx);
/** @} */

/** @name Toolkit Tests */
/** @{ */
/** @brief Run IZM construction and solver tests. */
int TEST_IZM(int verbose);
/** @brief Run VX segment tests. */
int TEST_VX_SEG(int verbose);
/** @} */

/** @} */

#endif // IZ_TOOLKIT_H
