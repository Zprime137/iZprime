#include <iZ_api.h>

const int s_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
                        43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
int s_primes_count = sizeof(s_primes) / sizeof(int);

// compute k of vx
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
 * @brief Initialize an iZm structure for a given vx size.
 *
 * @description:
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
    assert(vx % 2 != 0 && vx % 3 != 0 && vx >= 35 && "vx must be at least 35.");

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
 * @brief Calculate vx for a given range x_n by multiplying max_k small primes (> 3).
 *
 * Parameters:
 * @param x_n the number of bits to be vectorized
 * @param max_k the number of primes to be multiplied
 *
 * @return vx the product of the first max_k primes (> 3) not exceeding x_n
 */
uint64_t iZm_compute_limited_vx(uint64_t n, int max_k)
{

    uint64_t x_n = n / 6;
    uint64_t vx = 35; // minimum vx size
    int k = 4;        // pointing to prime 11 in s_primes

    // while vx * k-th prime doesn't exceed x_n
    while (vx * s_primes[k] < x_n && k < max_k + 2)
    {
        vx *= s_primes[k];
        k++;
    }

    return vx;
}

/**
 * @brief Compute the maximum vx such that vx < 2^bit_size.
 *
 * Parameters:
 * @param vx      mpz_t variable to store the computed maximum vx value.
 * @param bit_size Integer representing the bit size limit for vx.
 */
void iZm_compute_max_vx(mpz_t vx, int bit_size)
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
 * @brief Constructs a pre-sieved iZm base segment of size vx.
 *
 * @description:
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
 * @brief Compute the first composite hit of p in the yth vx segment in iZm index space.
 *
 * @description:
 * This function solves for the smallest x that satisfies:
 * (x + vx * y) ≡ x_p (mod p),
 * where x_p is normalized based on the m_id
 * and p to either x_p or p - x_p, then computes x and returns it.
 *
 * Parameters:
 * @param m_id  int indicating the matrix type (-1 for iZm5, 1 for iZm7).
 * @param p          Unsigned 64-bit integer parameter.
 * @param vx         size_t parameter representing the vx value.
 * @param y          Unsigned 64-bit integer parameter.
 *
 * @return The computed x value as a 64-bit unsigned integer.
 */
uint64_t iZm_solve_for_xp(int m_id, uint64_t p, uint64_t vx, uint64_t y)
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
 * @brief Solve for x given m_id, p, vx, and y using GMP.
 * Same as above but using GMP. Suitable for arbitrary y values.
 *
 * Parameters:
 * @param m_id  int indicating the target matrix (-1 for iZm5, 1 for iZm7).
 * @param p          Unsigned 64-bit integer parameter.
 * @param vx         Size_t parameter representing the vx value.
 * @param y          mpz_t parameter representing the y value.
 *
 * @return The computed x value as a 64-bit unsigned integer.
 */
uint64_t iZm_solve_for_xp_mpz(int m_id, uint64_t p, uint64_t vx, mpz_t y)
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
 * @brief Compute the first composite hit of p in the xth vy segment in iZm of width vx.
 *
 * @description:
 * This function solves for the smallest y that satisfies the equation
 * (x + vx * y) ≡ x_p (mod p), where x_p is normalized based on the m_id
 * and p. The function checks if p and vx are co-primes, and if not,
 * it returns -1 indicating no solution can be found. If they are co-primes,
 * it calculates the multiplicative inverse of vx modulo p and computes
 * y using the formula y = (delta * vx_inv) % p, where delta is the
 * difference between x_p and x modulo p.
 *
 * Parameters:
 * @param m_id  Integer indicating the matrix type (-1 for iZm5, 1 for iZm7).
 * @param p          Unsigned 64-bit integer parameter.
 * @param vx         Size_t parameter representing the vx value.
 * @param x          Unsigned 64-bit integer parameter.
 *
 * @return The computed y value as a 64-bit unsigned integer.
 */
int64_t iZm_solve_for_yp(int m_id, uint64_t p, uint64_t vx, uint64_t x)
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
