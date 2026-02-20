/**
 * @file iZ_toolkit.c
 * @brief Implementation of iZ index space helpers and iZm/VX segment machinery.
 * @ingroup iz_toolkit
 */

#include <iZ_api.h>

/** Small primes used to compute and construct wheel structures. */
static const int s_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
                               43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
/** Number of entries available in `s_primes`. */
static int s_primes_count = sizeof(s_primes) / sizeof(int);

/**
 * @brief Computes 6x + i for a given x and i.
 *
 * Parameters:
 * @param x (uint64_t) The value of `x` in `6x + i`.
 * @param i (int) The value of `i` in `6x + i`.
 *
 * @return The computed value `6x + i` as a 64-bit unsigned integer.
 */
inline uint64_t iZ(uint64_t x, int i)
{
    return 6 * x + i;
}

/**
 * @brief Computes 6x + i for arbitrary precision values using GMP.
 *
 * Parameters:
 * @param z (mpz_t) The result of the calculation `6x + i`.
 * @param x (mpz_t) The input value of `x` in `6x + i`.
 * @param i (int) The value of `i` in `6x + i`.
 */
void iZ_mpz(mpz_t z, mpz_t x, int i)
{
    mpz_mul_ui(z, x, 6); // z = 6 * x

    if (i > 0)
        mpz_add_ui(z, z, i); // z = z + i
    else
        mpz_sub_ui(z, z, -i); // z = z - i
}

/**
 * @brief Count how many small primes (>3) divide vx.
 * @param iZm Initialized toolkit context.
 * @return Number of pre-sieved small primes encoded in vx.
 */
static int compute_k_vx(IZM *iZm)
{
    int k = 0;
    while (iZm->vx % s_primes[k + 2] == 0)
    {
        k++;
    }
    return k;
}

/**
 * @ingroup iz_toolkit
 * @brief Consume iZ candidate bitmaps, emit primes, and mark root composites.
 * @param primes Destination array for discovered primes.
 * @param x5 Bitmap for 6x-1 candidates.
 * @param x7 Bitmap for 6x+1 candidates.
 * @param x_limit Exclusive x upper bound.
 */
void process_iZ_bitmaps(UI64_ARRAY *primes, BITMAP *x5, BITMAP *x7, uint64_t x_limit)
{
    uint64_t root_limit = sqrt(6 * x_limit) + 1;

    // Iterate through x values in range 0 < x < x_n
    for (uint64_t x = 1; x < x_limit; x++)
    {
        // if x5[x], implying it's iZ- prime
        if (bitmap_get_bit(x5, x))
        {
            uint64_t p = iZ(x, -1); // compute p = iZ(x, -1)
            ui64_push(primes, p);   // add p to primes

            // if p is root prime, mark its multiples in x5, x7
            if (p < root_limit)
            {
                bitmap_clear_steps_simd(x5, p, p * x + x, x_limit);
                bitmap_clear_steps_simd(x7, p, p * x - x, x_limit);
            }
        }

        // Do the same if x7[x], inverting the xp signs
        if (bitmap_get_bit(x7, x))
        {
            uint64_t p = iZ(x, 1);
            ui64_push(primes, p);

            if (p < root_limit)
            {
                bitmap_clear_steps_simd(x5, p, p * x - x, x_limit);
                bitmap_clear_steps_simd(x7, p, p * x + x, x_limit);
            }
        }
    }
}

/**
 * @ingroup iz_toolkit
 * @brief Generate root primes up to @p limit using iZ bitmap traversal.
 * @param primes Destination array.
 * @param limit Numeric upper bound.
 */
void get_root_primes(UI64_ARRAY *primes, uint64_t limit)
{
    // Add 2, 3 to primes
    ui64_push(primes, 2);
    ui64_push(primes, 3);

    // Calculate x_n, max x value in iZ space for given n
    uint64_t x_n = limit / 6 + 1;

    // Create bitmap X-Arrays x5, x7, each of size x_n + 1 bits
    BITMAP *x5 = bitmap_init(x_n + 1, 1);
    BITMAP *x7 = bitmap_init(x_n + 1, 1);

    // Memory allocation failed, check logs
    if (!x5 || !x7)
    {
        ui64_free(&primes);
        return;
    }

    // Sieve logic: mark composites and collect primes up to limit
    process_iZ_bitmaps(primes, x5, x7, x_n);

    // Cleanup: free memory of x5, x7
    bitmap_free(&x5);
    bitmap_free(&x7);

    return;
}

/**
 * @ingroup iz_toolkit
 * @brief Check the primality of a number using GMP's probabilistic test.
 * @param n Number to check.
 * @param rounds Number of Miller-Rabin rounds.
 * @return Non-zero if probably prime, 0 if composite.
 *
 * This function serves as a single source of truth for primality testing,
 * wrapping GMP's `mpz_probab_prime_p` function, allowing for changes to
 * the underlying primality testing method in the future without affecting the API.
 */
int check_primality(mpz_t n, int rounds)
{
    // GMP's mpz_probab_prime_p returns:
    // 0 if n is composite,
    // 1 if n is probably prime (without being certain),
    // 2 if n is definitely prime (only for small n).
    int is_prime = mpz_probab_prime_p(n, rounds);
    // ToDo: implement Baillie–PSW primality test for a deterministic alternative to Miller-Rabin, which would return a boolean result instead of a probability.

    return is_prime;
}

// =========================================================
// * iZm structure:
// =========================================================

/**
 * @ingroup iz_toolkit
 * @brief Initialize an iZm structure for a given vx size.
 *
 * @details
 * This function initializes an iZm structure for a specified vx size.
 * It allocates memory for the structure, generates root primes for
 * deterministic sieving, initializes base bitmaps, and constructs
 * pre-sieved base segments for iZm5 and iZm7.
 *
 * Parameters:
 * @param vx The size of the segment for the iZm structure.
 *
 * @return A pointer to the initialized IZM structure.
 */
IZM *iZm_init(size_t vx)
{
    // validate vx
    assert(vx >= 35 && "vx must be at least 35.");

    IZM *iZm = malloc(sizeof(IZM));
    if (!iZm)
    {
        log_error("Memory allocation failed for iZm.");
        return NULL;
    }

    iZm->vx = vx;
    // get root primes for deterministic sieving
    iZm->root_primes = SiZ(vx);
    // iZm->root_primes = root_limit < pow(10, 7) ? SiZ(root_limit) : SiZm(root_limit);
    if (!iZm->root_primes)
    {
        log_error("Root primes generation failed for iZm.");
        free(iZm);
        return NULL;
    }

    iZm->k_vx = compute_k_vx(iZm);

    // initialize base bitmaps
    iZm->base_x5 = bitmap_init(vx + 10, 1);
    iZm->base_x7 = bitmap_init(vx + 10, 1);
    if (!iZm->base_x5 || !iZm->base_x7)
    {
        log_error("Bitmap initialization failed for iZm base segment.");
        iZm_free(&iZm);
        ui64_free(&iZm->root_primes);
        return NULL;
    }

    // construct pre-sieved base_x5, base_x7 bitmaps
    iZm_construct_vx_base(vx, iZm->base_x5, iZm->base_x7);

    return iZm;
}

/**
 * @ingroup iz_toolkit
 * @brief Deep-copy an IZM object for independent worker usage.
 * @param src Source IZM object.
 * @return Cloned IZM object, or NULL on failure.
 */
IZM *iZm_clone(IZM *src)
{
    assert(src && "Invalid source IZM structure for cloning.");

    IZM *clone = malloc(sizeof(IZM));
    assert(clone && "Memory allocation failed for iZm clone.");

    clone->vx = src->vx;
    clone->k_vx = src->k_vx;
    clone->root_primes = ui64_init(src->root_primes->capacity);
    if (clone->root_primes == NULL)
    {
        log_error("Memory allocation failed for root primes in iZm clone.");
        iZm_free(&clone);
        return NULL;
    }
    // copy root primes
    memcpy(clone->root_primes->array, src->root_primes->array, src->root_primes->count * sizeof(uint64_t));
    clone->root_primes->count = src->root_primes->count;

    // clone base bitmaps
    clone->base_x5 = bitmap_clone(src->base_x5);
    clone->base_x7 = bitmap_clone(src->base_x7);

    return clone;
}

/**
 * @ingroup iz_toolkit
 * @brief Free the memory allocated for an IZM structure.
 *
 * Parameters:
 * @param iZm A pointer to the IZM structure to be freed.
 */
void iZm_free(IZM **iZm)
{
    if (iZm == NULL || *iZm == NULL)
        return;

    // Free root primes object allocated in iZm_init
    ui64_free(&(*iZm)->root_primes);
    bitmap_free(&(*iZm)->base_x5);
    bitmap_free(&(*iZm)->base_x7);
    free(*iZm);
    *iZm = NULL;
}

/**
 * @ingroup iz_toolkit
 * @brief Calculate a VX_{k} as the product of the first k primes > 3.
 *
 * @param k number of consecutive primes (> 3) to include in the product.
 * @return Computed VX_k value.
 */
uint64_t compute_vx_k(int k)
{
    uint64_t vx = 1;
    k += 2;    // adjust k to account for skipping 2 and 3
    int i = 2; // start from prime 5

    while (i < k && s_primes[i] * vx < UINT64_MAX)
    {
        vx *= s_primes[i++];
    }

    return vx;
}

/**
 * @ingroup iz_toolkit
 * @brief Choose a cache-aware VX size based on detected L2 capacity.
 * @param n Target numeric sieve bound.
 * @return L2 compatible VX size.
 */
uint64_t compute_l2_vx(uint64_t n)
{
    uint64_t l2 = get_cpu_L2_cache_size_bits();
    uint64_t x_n = n / 6;
    uint64_t vx = 35; // minimum useful vx size
    int k = 4;        // pointing to prime 11 in s_primes

    // while vx * k-th prime doesn't exceed x_n and l2 cache size
    while (vx * s_primes[k] < MIN(l2, x_n))
    {
        vx *= s_primes[k];
        k++;
    }

    return vx;
}

/**
 * @ingroup iz_toolkit
 * @brief Compute the maximum vx such that vx < 2^bit_size.
 *
 * Parameters:
 * @param vx      mpz_t variable to store the computed maximum vx value.
 * @param bit_size Integer representing the bit size limit for vx.
 */
void compute_max_vx(mpz_t vx, int bit_size)
{
    // get some primes to compute the primorial vx
    UI64_ARRAY *primes = SiZ(10000);
    assert(primes && "Failed to generate primes");

    int i = 2;                        // to skip 2, 3
    mpz_set_ui(vx, primes->array[i]); // set vx = 5

    // while vx * primes->array[i] < 2^bit_size
    while (mpz_sizeinbase(vx, 2) < (size_t)bit_size)
    {
        i++;
        mpz_mul_ui(vx, vx, primes->array[i]);
    }

    mpz_div_ui(vx, vx, primes->array[i]); // divide by the last prime
}

/**
 * @ingroup iz_toolkit
 * @brief Constructs a pre-sieved iZm base segment of size vx.
 *
 * Description:
 * This function constructs a pre-sieved iZm base segment of size vx. It marks
 * all composites of small primes that divide vx in the bitmaps base_x5 (iZ-) and
 * base_x7 (iZ+). This pre-sieved base can then be used as a template for sieving
 * all vx segments.
 *
 * Parameters:
 * @param iZm Pointer to the IZM structure containing the base segment bitmaps.
 */
void iZm_construct_vx_base(uint64_t vx, BITMAP *base_x5, BITMAP *base_x7)
{
    assert(base_x5 && base_x7 && "Invalid bitmaps passed to iZm_construct_vx_base.");

    bitmap_set_all(base_x5);
    bitmap_set_all(base_x7);
    bitmap_clear_bit(base_x5, 0); // clear the 0th bit
    bitmap_clear_bit(base_x7, 0); // clear the 0th bit

    // iterate through small primes that divide vx
    for (int i = 2; i < s_primes_count; i++)
    {
        int p = s_primes[i];
        if (vx % p != 0)
            continue;

        int ip = (p % 6 == 1) ? 1 : -1;
        int xp = (p + 1) / 6;
        // clear composites in iZ- and iZ+
        if (ip == -1)
        {
            bitmap_clear_bit(base_x5, xp);                        // clear the prime itself
            bitmap_clear_steps_simd(base_x5, p, p * xp + xp, vx); // mark composites in iZ-
            bitmap_clear_steps_simd(base_x7, p, p * xp - xp, vx); // mark composites in iZ+
        }
        else
        {
            bitmap_clear_bit(base_x7, xp);                        // clear the prime itself
            bitmap_clear_steps_simd(base_x5, p, p * xp - xp, vx); // mark composites in iZ-
            bitmap_clear_steps_simd(base_x7, p, p * xp + xp, vx); // mark composites in iZ+
        }
    }
}

/**
 * @ingroup iz_toolkit
 * @brief Compute the first composite hit of p in the yth vx segment in iZm index space.
 *
 * Description:
 * This function solves for the smallest x that satisfies:
 * (x + vx * y) ≡ x_p (mod p),
 * where x_p is normalized based on the m_id
 * and p to either x_p or p - x_p, then computes x and returns it.
 *
 * Parameters:
 * @param m_id       int matrix identifier (-1 for iZm5, 1 for iZm7).
 * @param p          Unsigned 64-bit integer parameter.
 * @param vx         size_t parameter representing the vx value.
 * @param y          Unsigned 64-bit integer parameter.
 *
 * @return The computed x value as a 64-bit unsigned integer.
 */
uint64_t iZm_solve_for_x0(int m_id, uint64_t p, uint64_t vx, uint64_t y)
{
    uint64_t xp = (p + 1) / 6;
    int ip = (p % 6 == 1) ? 1 : -1;

    // Immediate solution check for y = 0
    if (y == 0)
    {
        // compute p * xp + m_id * ip * xp
        return p * xp + m_id * ip * xp;
    }

    // Normalize x_p to x_p if p_id = m_id, else to p - x_p
    xp = (m_id == ip) ? xp : p - xp;
    uint64_t yvx = vx * y;

    // Compute the first composite hit of p in the yth vx segment in iZm index space
    uint64_t x = p < vx ? p - (yvx - xp) % p : (yvx - xp) % p;
    return x;
}

/**
 * @ingroup iz_toolkit
 * @brief Solve for x given m_id, p, vx, and y using GMP.
 * Same as above but using GMP. Suitable for arbitrary y values.
 *
 * Parameters:
 * @param m_id       int matrix identifier (-1 for iZm5, 1 for iZm7).
 * @param p          Unsigned 64-bit integer parameter.
 * @param vx         Size_t parameter representing the vx value.
 * @param y          mpz_t parameter representing the y value.
 *
 * @return The computed x value as a 64-bit unsigned integer.
 */
uint64_t iZm_solve_for_x0_mpz(int m_id, uint64_t p, uint64_t vx, mpz_t y)
{
    mpz_t tmp;
    mpz_init(tmp);

    // 1. Normalize x_p to x_p if p_id = m_id, else to p - x_p
    uint64_t xp = (p + 1) / 6;
    int ip = (p % 6 == 1) ? 1 : -1;
    xp = (m_id == ip) ? xp : p - xp;

    mpz_mul_ui(tmp, y, vx);
    mpz_sub_ui(tmp, tmp, xp);
    mpz_mod_ui(tmp, tmp, p);

    uint64_t x = p < vx ? p - mpz_get_ui(tmp) : mpz_get_ui(tmp);

    mpz_clear(tmp);
    return x;
}

/**
 * @ingroup iz_toolkit
 * @brief Compute the first composite hit of p in the xth vy segment in iZm of width vx.
 *
 * Description:
 * This function solves for the smallest y that satisfies the equation
 * (x + vx * y) ≡ x_p (mod p), where x_p is normalized based on the m_id
 * and p. The function checks if p and vx are co-primes, and if not,
 * it returns -1 indicating no solution can be found. If they are co-primes,
 * it calculates the multiplicative inverse of vx modulo p and computes
 * y using the formula y = (delta * vx_inv) % p, where delta is the
 * difference between x_p and x modulo p.
 *
 * Parameters:
 * @param m_id       int matrix identifier (-1 for iZm5, 1 for iZm7).
 * @param p          Unsigned 64-bit integer parameter.
 * @param vx         Size_t parameter representing the vx value.
 * @param x          Unsigned 64-bit integer parameter.
 *
 * @return The computed y value as a 64-bit unsigned integer.
 */
int64_t iZm_solve_for_y0(int m_id, uint64_t p, uint64_t vx, uint64_t x)
{
    // No solution check, if vx and p are not coprime
    if (gcd(vx, p) != 1)
    {
        printf("There's no solution for the given parameters\n");
        return -1; // No solution can be found
    }

    uint64_t xp = (p + 1) / 6;
    int ip = (p % 6 == 1) ? 1 : -1;
    xp = (m_id == ip) ? xp : p - xp;

    // Immediate solution check
    if (x % p == xp)
        return 0;

    // Compute delta
    uint64_t x_mod_p = x % p;
    uint64_t delta = (xp + p - x_mod_p) % p;

    // Find modular inverse
    uint64_t vx_inv = modular_inverse(vx % p, p);

    // Compute and return y
    uint64_t y = (delta * vx_inv) % p;
    return y;
}

// ===================================================
// * VX_SEG structure:
// ===================================================

/**
 * @brief Initialize mpz-dependent base fields for a VX segment object.
 * @param vx_obj Segment object to populate.
 * @param y_str Decimal y-coordinate for the segment.
 * @return 1 on success, 0 on parse/initialization failure.
 */
static int vx_set_base_values(VX_SEG *vx_obj, char *y_str)
{

    // assert y_str is numeric
    mpz_t y_tmp;
    mpz_init(y_tmp);
    if (mpz_set_str(y_tmp, y_str, 10) != 0)
    {
        log_error("Invalid numeric string for y in vx_set_base_values\n");
        mpz_clear(y_tmp);
        return 0;
    }

    mpz_init_set(vx_obj->y, y_tmp);
    mpz_clear(y_tmp);

    // Compute yvx = y * vx
    mpz_init(vx_obj->yvx);
    mpz_mul_ui(vx_obj->yvx, vx_obj->y, vx_obj->vx);

    // Compute root_limit = sqrt(iZ(vx * (y+1), 1))
    mpz_init(vx_obj->root_limit);
    mpz_add_ui(vx_obj->root_limit, vx_obj->yvx, vx_obj->vx);
    iZ_mpz(vx_obj->root_limit, vx_obj->root_limit, 1);
    mpz_sqrt(vx_obj->root_limit, vx_obj->root_limit);

    // Set is_large_limit to determine if probabilistic primality test is needed
    // if root_limit > vx
    vx_obj->is_large_limit = mpz_cmp_ui(vx_obj->root_limit, vx_obj->vx) > 0;
    return 1;
}

/**
 * @brief Deterministic phase: mark composites using root primes.
 * @param iZm Toolkit context containing root-prime table and base bitmaps.
 * @param vx_obj Segment object to update.
 */
static void vx_det_sieve(IZM *iZm, VX_SEG *vx_obj)
{
    assert(iZm && "iZm is NULL in vx_det_sieve");
    assert(vx_obj && "vx_obj is NULL in vx_det_sieve");

    int vx = vx_obj->vx; // segment size

    // * Deterministic Sieve: Mark composites of primes < vx in x5, x7
    int start_x = vx_obj->start_x;
    int end_x = vx_obj->end_x;
    int k = 2 + iZm->k_vx; // skip 2, 3 and pre-sieved k_vx primes
    UI64_ARRAY *root_primes = iZm->root_primes;

    // if y < 2^64, use iZm_solve_for_x0 version for efficiency
    if (mpz_sizeinbase(vx_obj->y, 2) <= 64)
    {
        uint64_t y = mpz_get_ui(vx_obj->y);
        uint64_t root_limit = mpz_get_ui(vx_obj->root_limit);

        // Iterate through root primes, skipping the first k pre-sieved primes
        for (int i = k; i < root_primes->count; i++)
        {
            uint64_t p = root_primes->array[i];

            if (p > root_limit)
                break;

            // Mark composites of p in x5 and x7
            bitmap_clear_steps_simd(vx_obj->x5, p, iZm_solve_for_x0(-1, p, vx, y), end_x);
            bitmap_clear_steps_simd(vx_obj->x7, p, iZm_solve_for_x0(1, p, vx, y), end_x);

            vx_obj->bit_ops += (2 * end_x) / p; // approximate number of bit operations
        }
    }
    else
    {
        // the same as above but using iZm_solve_for_x0_mpz version
        for (int i = k; i < root_primes->count; i++)
        {
            int p = root_primes->array[i];

            bitmap_clear_steps_simd(vx_obj->x5, p, iZm_solve_for_x0_mpz(-1, p, vx, vx_obj->y), end_x);
            bitmap_clear_steps_simd(vx_obj->x7, p, iZm_solve_for_x0_mpz(1, p, vx, vx_obj->y), end_x);

            vx_obj->bit_ops += (2 * end_x) / p;
        }
    }

    // if no further probabilistic testing is needed,
    // count unmarked bits in x5 and x7 as p_count
    if (!vx_obj->is_large_limit)
    {
        vx_obj->p_count = 0;
        for (int x = start_x; x <= end_x; x++)
        {
            if (bitmap_get_bit(vx_obj->x5, x))
                vx_obj->p_count++;
            if (bitmap_get_bit(vx_obj->x7, x))
                vx_obj->p_count++;
        }
    }
}

/**
 * @brief Perform probabilistic sieve cleanup for large numeric ranges.
 * @param vx_obj Segment object containing deterministic survivors.
 */
static void vx_prob_sieve(VX_SEG *vx_obj)
{
    // If is_large_limit is false, no need for probabilistic testing
    if (!vx_obj->is_large_limit)
    {
        log_info("vx_obj->is_large_limit is false, no need for probabilistic testing");
        return;
    }

    // Initialize GMP reusable variables p, x_p
    mpz_t p, x_p;
    mpz_init(p);
    mpz_init(x_p);

    int r = vx_obj->mr_rounds;
    int s = vx_obj->start_x <= 1 ? 1 : vx_obj->start_x;

    // Iterate through x values in the range start_x <= x <= end_x
    for (int x = s; x <= vx_obj->end_x; x++)
    {
        // Check if iZ(vx * y + x, -1) is prime, if not, clear x in x5
        if (bitmap_get_bit(vx_obj->x5, x))
        {
            // Compute x_p = yvx + x
            mpz_add_ui(x_p, vx_obj->yvx, x);
            iZ_mpz(p, x_p, -1); // Compute p = iZ(x_p, -1)
            int is_prime = check_primality(p, r);
            vx_obj->p_test_ops++;

            // if is_prime, increment count, else clear composite from x5
            if (is_prime)
            {
                vx_obj->p_count++;
            }
            else
            {
                bitmap_clear_bit(vx_obj->x5, x); // Clear composite from x5
            }
        }

        // same for the 6x+1 candidate
        if (bitmap_get_bit(vx_obj->x7, x))
        {
            mpz_add_ui(x_p, vx_obj->yvx, x);
            iZ_mpz(p, x_p, 1); // Compute p = iZ(x_p, 1)
            int is_prime = check_primality(p, r);
            vx_obj->p_test_ops++;

            if (is_prime)
            {
                vx_obj->p_count++;
            }
            else
            {
                bitmap_clear_bit(vx_obj->x7, x); // Clear composite from x7
            }
        }
    }

    vx_obj->is_large_limit = 0; // all composites cleared

    // Cleanup
    mpz_clears(p, x_p, NULL);
}

/**
 * @ingroup iz_toolkit
 * @brief Initialize the members of the VX_SEG structure with the given parameters and defaults.
 *
 * Description:
 * This function allocates memory for a VX_SEG structure and initializes its members.
 * The count is initialized to 0, and the p_gaps array is allocated with
 * an initial size of (vx/2).
 *
 * Parameters:
 * @param y_str A character pointer representing a numeric string.
 *
 * @return VX_SEG* A pointer to the initialized VX_SEG structure.
 *        NULL if memory allocation fails or y_str is not a numeric string.
 */
VX_SEG *vx_init(IZM *iZm, int start_x, int end_x, char *y_str, int mr_rounds)
{
    // assert vx > 5 and not a multiple of 2 or 3
    assert(iZm && "iZm is NULL in vx_init");
    assert(y_str && "y_str is NULL in vx_init");

    VX_SEG *vx_obj = malloc(sizeof(VX_SEG));
    if (vx_obj == NULL)
    {
        log_error("Memory allocation failed in vx_init\n");
        return NULL;
    }

    // Initialize struct members
    vx_obj->vx = iZm->vx;

    // Set base values
    if (!vx_set_base_values(vx_obj, y_str))
    {
        // check logs
        free(vx_obj);
        return NULL;
    }

    vx_obj->mr_rounds = (mr_rounds == 0) ? MR_ROUNDS : mr_rounds; // default 25 rounds

    vx_obj->start_x = MAX(start_x, 1);
    vx_obj->end_x = MIN(end_x, vx_obj->vx);
    vx_obj->x5 = bitmap_clone(iZm->base_x5);
    vx_obj->x7 = bitmap_clone(iZm->base_x7);
    vx_obj->p_count = 0;
    vx_obj->p_gaps = NULL;
    vx_obj->bit_ops = 0;
    vx_obj->p_test_ops = 0;

    // perform deterministic sieving to prepare for probabilistic sieving or streaming
    vx_det_sieve(iZm, vx_obj);

    return vx_obj;
}

/**
 * @ingroup iz_toolkit
 * @brief Free all memory owned by a VX segment object.
 * @param vx_obj Address of the VX_SEG pointer to release.
 */
void vx_free(VX_SEG **vx_obj)
{
    if (vx_obj == NULL || *vx_obj == NULL)
        return;

    // clear bitmaps
    bitmap_free(&(*vx_obj)->x5);
    bitmap_free(&(*vx_obj)->x7);

    // clear p_gaps array
    ui16_free(&(*vx_obj)->p_gaps);

    // clear mpz_t variables
    mpz_clears((*vx_obj)->y, (*vx_obj)->yvx, (*vx_obj)->root_limit, NULL);

    free(*vx_obj);
    *vx_obj = NULL;
}

/**
 * @ingroup iz_toolkit
 * @brief Convert survivor bits into compact prime-gap encoding.
 *
 * This routine must run after deterministic/probabilistic sieving is complete.
 * It populates @ref VX_SEG::p_gaps for downstream streaming and gap traversal.
 *
 * @param vx_obj Segment object with validated prime survivors.
 */
void vx_collect_p_gaps(VX_SEG *vx_obj)
{
    assert(vx_obj && vx_obj->p_count > 0 && "Invalid vx_obj in vx_collect_p_gaps");
    assert(mpz_cmp_ui(vx_obj->y, 0) > 0 && "First segment requires special handling in vx_collect_p_gaps");
    if (vx_obj->is_large_limit)
    {
        vx_full_sieve(vx_obj, 0);
    }

    vx_obj->p_gaps = ui16_init(vx_obj->p_count + 2);
    assert(vx_obj->p_gaps && "Memory allocation failed for vx_obj->p_gaps in vx_collect_p_gaps");

    // Initialize gap counter
    int gap = 0;

    // Iterate through x values in the range start_x <= x <= end_x
    for (int x = vx_obj->start_x; x <= vx_obj->end_x; x++)
    {
        // Increment gap by 4 since: iZ(x, -1) - iZ(x-1, 1) = 4
        gap += 4;

        // Check if iZ(vx * y + x, -1) is prime
        if (bitmap_get_bit(vx_obj->x5, x))
        {
            // Append gap to p_gaps
            ui16_push(vx_obj->p_gaps, gap);
            gap = 0; // Reset gap
        }

        // Increment gap by 2
        gap += 2;

        // Check if iZ(vx * y + x, 1) is prime
        if (bitmap_get_bit(vx_obj->x7, x))
        {
            // Append gap to p_gaps
            ui16_push(vx_obj->p_gaps, gap);
            gap = 0; // Reset gap
        }
    }

    // append final gap for backward calculations
    ui16_push(vx_obj->p_gaps, gap);
}

/**
 * @ingroup iz_toolkit
 * @brief This function performs the sieve process on a given vx and y defined
 * in the VX_SEG structure, and stores the primes gaps in the vx_obj->p_gaps array.
 *
 * Description: This function combines deterministic sieving and probabilistic
 * primality tests to identify prime candidates in a standard VX segment of a
 * specific y in the iZ-Matrix. It could be used to only count primes in the
 * vx segment, and optionally populates the vx_obj->p_gaps array with prime gaps
 * between consecutive primes detected in the segment.
 *
 * @param vx_obj The VX_SEG to be processed.
 * @param collect_p_gaps 0 only counts primes and doesn't store prime gaps,
 * other non-zero int values populates vx_obj->p_gaps with detected prime gaps.
 */
void vx_full_sieve(VX_SEG *vx_obj, int collect_p_gaps)
{
    assert(vx_obj && "vx_obj is NULL in vx_full_sieve");

    // If is_large_limit is true, perform probabilistic primality tests
    if (vx_obj->is_large_limit)
        vx_prob_sieve(vx_obj);

    // If collect_p_gaps is true, populate the p_gaps array
    if (collect_p_gaps)
        vx_collect_p_gaps(vx_obj);
}

/**
 * @ingroup iz_toolkit
 * @brief Stream segment primes to an output stream in traversal order.
 * @param vx_obj Segment object.
 * @param output Destination stream (stdout or file).
 */
void vx_stream(VX_SEG *vx_obj, FILE *output)
{
    assert(vx_obj && "vx_obj is NULL in vx_stream");
    assert(output && "output stream is NULL in vx_stream");

    // Initialize GMP reusable variables p, x_p
    mpz_t p, x_p;
    mpz_init(p);
    mpz_init(x_p);

    int r = vx_obj->mr_rounds;

    // Iterate through x values in the range start_x <= x <= end_x
    for (int x = vx_obj->start_x; x <= vx_obj->end_x; x++)
    {
        // Check if iZ(vx * y + x, -1) is prime, if not, clear x in x5
        if (bitmap_get_bit(vx_obj->x5, x))
        {
            // Compute x_p = yvx + x
            mpz_add_ui(x_p, vx_obj->yvx, x);
            iZ_mpz(p, x_p, -1); // Compute p = iZ(x_p, -1)
            int is_prime = 1;

            if (vx_obj->is_large_limit)
            {
                vx_obj->p_test_ops++;
                is_prime = check_primality(p, r);
            }

            if (is_prime)
            {
                if (vx_obj->is_large_limit)
                {
                    vx_obj->p_count++; // otherwise already counted in det_sieve
                }
                gmp_fprintf(output, "%Zd ", p);
            }
            else
            {
                bitmap_clear_bit(vx_obj->x5, x); // Clear composite from x5
            }
        }

        // test iZ(vx * y + x, 1) for primality
        if (bitmap_get_bit(vx_obj->x7, x))
        {
            mpz_add_ui(x_p, vx_obj->yvx, x);
            iZ_mpz(p, x_p, 1); // Compute p = iZ(x_p, 1)
            int is_prime = 1;

            if (vx_obj->is_large_limit)
            {
                vx_obj->p_test_ops++;
                is_prime = check_primality(p, r);
            }

            if (is_prime)
            {
                if (vx_obj->is_large_limit)
                {
                    vx_obj->p_count++;
                }
                gmp_fprintf(output, "%Zd ", p);
            }
            else
            {
                bitmap_clear_bit(vx_obj->x7, x); // Clear composite from x7
            }
        }
    }

    mpz_clears(p, x_p, NULL);
}

// ==================================================
// * IZM_RANGE_INFO structure:
// ==================================================

IZM_RANGE_INFO range_info_init(INPUT_SIEVE_RANGE *input_range, int vx)
{
    IZM_RANGE_INFO info = {0};
    info.vx = vx;
    info.y_range = -1;

    mpz_inits(info.Zs, info.Ze, info.Xs, info.Xe, info.Ys, info.Ye, NULL);

    if (!input_range || !input_range->start || vx < 35 || mpz_set_str(info.Zs, input_range->start, 10) != 0)
    {
        log_error("range_info_init: invalid input.");
        return info;
    }

    if (input_range->range == 0)
    {
        mpz_set(info.Ze, info.Zs);
    }
    else
    {
        // inclusive upper bound for range size semantics: [start, start + range - 1]
        mpz_add_ui(info.Ze, info.Zs, input_range->range - 1);
    }

    mpz_fdiv_q_ui(info.Xs, info.Zs, 6);
    mpz_fdiv_q_ui(info.Xe, info.Ze, 6);
    mpz_fdiv_q_ui(info.Ys, info.Xs, vx);
    mpz_fdiv_q_ui(info.Ye, info.Xe, vx);

    mpz_t y_delta;
    mpz_init(y_delta);
    mpz_sub(y_delta, info.Ye, info.Ys);

    if (mpz_sgn(y_delta) < 0 || !mpz_fits_sint_p(y_delta))
    {
        log_error("range_info_init: computed y-range is out of supported bounds.");
        mpz_clear(y_delta);
        return info;
    }

    info.y_range = (int)mpz_get_si(y_delta);
    mpz_clear(y_delta);

    return info;
}
void range_info_free(IZM_RANGE_INFO *info)
{
    if (info == NULL)
        return;

    mpz_clears(info->Zs, info->Ze, info->Xs, info->Xe, info->Ys, info->Ye, NULL);
}

// ==================================================
// * iZm Search primes routines:
// ==================================================

/**
 * @ingroup iz_toolkit
 * @brief horizontal search routine for generating a random prime.
 *
 * Description: This function searches for a prime number using the given parameters.
 * It combines the iZ-Matrix filtering techniques with the Miller-Rabin primality test.
 * The search is performed by finding suitable x values that do not correspond to
 * composites of primes that divide vx. Then, it iterates over y values in the
 * equation p = iZ(x + vx * y) until a prime is found.
 *
 * @param p The prime number found in the search.
 * @param m_id The identifier (1 or -1) for the iZ matrix.
 * @param vx The horizontal vector of the iZ matrix.
 * @param bit_size The target bit size of the prime.
 * @return 1 if a prime is found, 0 otherwise.
 */
int vx_search_prime(mpz_t p, int m_id, int vx, int bit_size)
{
    // assert p is not NULL
    assert(p && "p cannot be NULL in vx_search_prime.");

    bit_size = MAX(bit_size, 10);

    // set m_id randomly if not provided
    if (m_id != -1 && m_id != 1)
        m_id = (rand() % 2) ? 1 : -1;

    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state);

    UI64_ARRAY *root_primes = SiZm(vx);
    if (!root_primes)
    {
        log_error("Failed to initialize root primes in vx_search_prime");
        gmp_randclear(state);
        return 0;
    }

    mpz_t z, x_z, y, yvx;
    mpz_init(z);
    mpz_init(x_z);
    mpz_init(y);
    mpz_init(yvx);

    mpz_urandomb(y, state, bit_size);
    mpz_fdiv_q_ui(y, y, 6 * vx);
    mpz_mul_ui(yvx, y, vx); // compute yvx = vx * y

    int found = 0;
    while (!found)
    {
        BITMAP *bitmap = bitmap_init(vx + 10, 1);

        // sieve the bitmap with root primes skipping 2 and 3
        for (int i = 2; i < root_primes->count; i++)
        {
            uint64_t q = root_primes->array[i];
            // mark composites of q in the bitmap
            bitmap_clear_steps(bitmap, q, iZm_solve_for_x0_mpz(m_id, q, vx, y), vx);
        }

        int random_x = rand() % (vx / 2); // random x < vx/2
        for (int x = random_x; x < vx; x++)
        {
            // check if z is prime candidate in bitmap
            if (bitmap_get_bit(bitmap, x))
            {
                // compute x_z = vx * y + x
                mpz_add_ui(x_z, yvx, x);
                iZ_mpz(z, x_z, m_id);
                // check if z is prime
                found = check_primality(z, MR_ROUNDS);

                // if z is prime, set p = z
                if (found)
                {
                    mpz_set(p, z);
                    break;
                }
            }
        }

        // cleanup
        bitmap_free(&bitmap);

        // increment y by 1 for next vx segment
        mpz_add_ui(y, y, 1);
        mpz_add_ui(yvx, yvx, vx);
    }

    ui64_free(&root_primes);
    mpz_clears(z, x_z, y, yvx, NULL);
    gmp_randclear(state);

    return found;
}

/**
 * @ingroup iz_toolkit
 * @brief vertical search routine for generating a random prime.
 *
 * Description: This function searches for a prime number using the given parameters.
 * It combines the iZ-Matrix filtering techniques with the Miller-Rabin primality test.
 * The search is performed by finding a suitable x value that does not correspond to
 * a composite of a prime that divides vx. Then, it iterates over y value in the
 * equation p = iZ(x + vx * y, m_id) until a prime is found.
 *
 * @param p The prime number found in the search.
 * @param m_id The identifier (1 or -1) for the iZ matrix.
 * @param vx The horizontal vector of the iZ matrix.
 * @return 1 if a prime is found, 0 otherwise.
 */
int vy_search_prime(mpz_t p, int m_id, mpz_t vx)
{
    assert(p && vx && "Input parameters cannot be NULL");

    int found = 0; // flag to indicate if a prime was found

    // set m_id randomly if not provided
    if (m_id != -1 && m_id != 1)
        m_id = (rand() % 2) ? 1 : -1;

    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state); // seed random state

    mpz_t z, g;
    mpz_init(z);
    mpz_init(g);

    // set random x value in the range of vx
    mpz_urandomm(z, state, vx);
    // compute z = 6 * z + i
    iZ_mpz(z, z, m_id);

    // search for x value such that gcd(iZ(x, m_id), z) = 1
    for (;;)
    {
        // increment z by 6 to advance x by 1
        mpz_add_ui(z, z, 6);

        // Compute g = gcd(vx, z)
        mpz_gcd(g, vx, z);

        // break if g = 1,
        // implying current x value can yield primes of the form iZ(x + vx * y, p_id)
        if (mpz_cmp_ui(g, 1) == 0)
            break;
    }

    // set g = 6 * vx to use as a step
    mpz_mul_ui(g, vx, 6);
    // add z by rand * g
    int rand_steps = rand() % 100;
    mpz_addmul_ui(z, g, rand_steps);

    while (!found)
    {
        // increment z by 6 * vx to advance y by 1
        mpz_add(z, z, g);

        // check if z is prime
        found = check_primality(z, MR_ROUNDS);

        // if z is prime, set p = z
        if (found)
            mpz_set(p, z);
    }

    // cleanup
    gmp_randclear(state);
    mpz_clears(z, g, NULL);

    return found;
}
