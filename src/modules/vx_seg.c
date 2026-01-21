/**
 * @file vx_seg.c
 * @brief VX_SEG module.
 *
 * @description:
 *
 */

#include <vx_seg.h>

// * VX_SEG:
// ===================================

int vx_set_base_values(VX_SEG *vx_obj, char *y_str)
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
 * @brief Perform deterministic sieving on the VX_SEG structure.
 *
 * @param vx_obj Pointer to the VX_SEG structure to be processed.
 */
void vx_det_sieve(IZM *iZm, VX_SEG *vx_obj)
{
    assert(iZm && "iZm is NULL in vx_det_sieve");
    assert(vx_obj && "vx_obj is NULL in vx_det_sieve");

    int vx = vx_obj->vx; // segment size

    // * Deterministic Sieve: Mark composites of primes < vx in x5, x7
    int start_x = vx_obj->start_x;
    int end_x = vx_obj->end_x;
    int k = 2 + iZm->k_vx; // skip 2, 3 and pre-sieved k_vx primes
    UI64_ARRAY *root_primes = iZm->root_primes;

    // if y < 2^64, use iZm_solve_for_xp version for efficiency
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
            bitmap_clear_steps_simd(vx_obj->x5, p, iZm_solve_for_xp(-1, p, vx, y), end_x);
            bitmap_clear_steps_simd(vx_obj->x7, p, iZm_solve_for_xp(1, p, vx, y), end_x);

            vx_obj->bit_ops += (2 * end_x) / p; // approximate number of bit operations
        }
    }
    else
    {
        // the same as above but using iZm_solve_for_xp_mpz version
        for (int i = k; i < root_primes->count; i++)
        {
            int p = root_primes->array[i];

            bitmap_clear_steps_simd(vx_obj->x5, p, iZm_solve_for_xp_mpz(-1, p, vx, vx_obj->y), end_x);
            bitmap_clear_steps_simd(vx_obj->x7, p, iZm_solve_for_xp_mpz(1, p, vx, vx_obj->y), end_x);

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
 * @brief Perform probabilistic sieving on the VX_SEG structure.
 *
 * @param vx_obj Pointer to the VX_SEG structure to be processed.
 */
void vx_prob_sieve(VX_SEG *vx_obj)
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
            int is_prime = mpz_probab_prime_p(p, r);
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
            int is_prime = mpz_probab_prime_p(p, r);
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
 * @brief Initialize the members of the VX_SEG structure with the given parameters and defaults.
 *
 * @description:
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
    // vx_obj->iZm = iZm;
    vx_obj->vx = iZm->vx;

    // Set base values
    if (!vx_set_base_values(vx_obj, y_str))
    {
        // check logs
        free(vx_obj);
        return NULL;
    }

    vx_obj->mr_rounds = (mr_rounds == 0) ? MR_ROUNDS : mr_rounds; // default 25 rounds

    int vx = vx_obj->vx;
    vx_obj->start_x = MAX(start_x, 1);
    vx_obj->end_x = MIN(end_x, vx);
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
 * @brief Free the VX_SEG structure.
 *
 * @description:
 * This function frees all memory associated with the VX_SEG structure.
 *
 * Parameters:
 * @param vx_obj Pointer to the VX_SEG structure to be freed.
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

// Important: should be used after sieving
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
 * @brief This function performs the sieve process on a given vx and y defined
 * in the VX_SEG structure, and stores the primes gaps in the vx_obj->p_gaps array.
 *
 * @description: This function combines deterministic sieving and probabilistic
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

// streams primes incrementally to the output file
void vx_stream_file(VX_SEG *vx_obj, FILE *output)
{
    assert(vx_obj && "vx_obj is NULL in vx_stream_file");
    assert(output && "output file is NULL in vx_stream_file");

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
                is_prime = mpz_probab_prime_p(p, r);
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
                is_prime = mpz_probab_prime_p(p, r);
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
