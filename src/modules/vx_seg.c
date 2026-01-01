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
VX_SEG *vx_init(int vx, char *y_str, int mr_rounds)
{
    // assert vx > 5 and not a multiple of 2 or 3
    assert(vx % 2 != 0 && vx % 3 != 0 && vx >= 35 && "vx must be at least 35.");
    assert(y_str && "y_str is NULL in vx_init");

    VX_SEG *vx_obj = malloc(sizeof(VX_SEG));
    if (vx_obj == NULL)
    {
        log_error("Memory allocation failed in vx_init\n");
        return NULL;
    }

    // Initialize struct members
    vx_obj->vx = vx;

    // Set base values
    if (!vx_set_base_values(vx_obj, y_str))
    {
        // check logs
        free(vx_obj);
        return NULL;
    }

    vx_obj->mr_rounds = (mr_rounds == 0) ? MR_ROUNDS : mr_rounds; // default 25 rounds
    vx_obj->start_x = 1;                                          // default starting index
    vx_obj->end_x = vx;                                           // default end index
    vx_obj->x5 = NULL;
    vx_obj->x7 = NULL;
    vx_obj->p_count = 0;
    vx_obj->p_gaps = NULL;
    vx_obj->bit_ops = 0;
    vx_obj->p_test_ops = 0;

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
    assert((vx_obj != NULL || *vx_obj != NULL) && "vx_obj is NULL in vx_free");

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

    vx_obj->p_gaps = ui16_init(vx_obj->p_count + 2);
    assert(vx_obj->p_gaps != NULL && "Memory allocation failed for vx_obj->p_gaps in vx_collect_p_gaps");

    // if vx_obj->y == 0, append gaps for pre-sieved primes
    int s = vx_obj->start_x == 0 ? 1 : vx_obj->start_x;
    if (mpz_cmp_ui(vx_obj->y, 0) == 0 && s == 1)
    {
        // append gaps for pre-sieved primes
        int pre_sieved[] = {5, 7, 11, 13, 17, 19, 23, 29};
        ui16_push(vx_obj->p_gaps, 1); // 2 from 1
        ui16_push(vx_obj->p_gaps, 1); // 3 from 2
        int vx = vx_obj->vx;
        int p = 3;
        int i = 0;
        while (vx % pre_sieved[i] == 0)
        {
            ui16_push(vx_obj->p_gaps, pre_sieved[i] - p);
            p = pre_sieved[i];
            i++;
        }
    }

    // Initialize gap counter
    int gap = 0;

    // Iterate through x values in the range start_x <= x <= end_x
    for (int x = s; x <= vx_obj->end_x; x++)
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

// Get the nth prime in this vx segment
int vx_nth_p(mpz_t p, VX_SEG *vx_obj, int n)
{
    assert(vx_obj != NULL && "vx_obj is NULL in vx_nth_p");
    assert(p != NULL && "p is NULL in vx_nth_p");
    assert(vx_obj->p_gaps != NULL && "vx_obj->p_gaps is NULL in vx_nth_p");

    if (abs(n) > vx_obj->p_count)
        return 0; // failure, n is out of bounds

    // n is positive, count from the beginning iZ(vx * y, 1)
    if (n > 0)
    {
        mpz_set(p, vx_obj->yvx);
        iZ_mpz(p, p, 1);
        // sum the first n gaps to get to the nth prime
        int gap_sum = 0;
        for (int i = 0; i < n; i++)
        {
            gap_sum += vx_obj->p_gaps->array[i];
        }
        mpz_add_ui(p, p, gap_sum);
    }
    else
    {
        // if n is negative, count from the end iZ(vx * (y+1), 1)
        mpz_add_ui(p, vx_obj->yvx, vx_obj->vx);
        iZ_mpz(p, p, 1);
        // sum the last |n| gaps to get to the nth prime from the end
        int gap_sum = 0;
        for (int i = vx_obj->p_gaps->count - n; i < vx_obj->p_gaps->count; i++)
        {
            gap_sum += vx_obj->p_gaps->array[i];
        }
        mpz_sub_ui(p, p, gap_sum);
    }

    return 1; // success
}

/**
 * @brief Perform deterministic sieving on the VX_SEG structure.
 *
 * @param iZm Pointer to the IZM structure containing sieve resources.
 * @param vx_obj Pointer to the VX_SEG structure to be processed.
 */
void vx_det_sieve(IZM *iZm, VX_SEG *vx_obj)
{
    assert(iZm && "iZm is NULL in vx_det_sieve");
    assert(vx_obj && "vx_obj is NULL in vx_det_sieve");

    // Initialize x5 and x7 bitmaps by copying from iZm base bitmaps
    vx_obj->x5 = bitmap_clone(iZm->base_x5);
    vx_obj->x7 = bitmap_clone(iZm->base_x7);

    int vx = vx_obj->vx; // segment size

    // * Deterministic Sieve: Mark composites of primes < vx in x5, x7
    int start_x = vx_obj->start_x;
    int end_x = vx_obj->end_x;
    int k = 2 + iZm->k_vx; // skip 2, 3 and pre-sieved k_vx primes

    // if y < 2^64, use iZm_solve_for_xp version for efficiency
    if (mpz_sizeinbase(vx_obj->y, 2) <= 64)
    {
        uint64_t y = mpz_get_ui(vx_obj->y);
        uint64_t root_limit = mpz_get_ui(vx_obj->root_limit);

        // Iterate through root primes, skipping 2, 3 and those that divide vx
        for (int i = k; i < iZm->root_primes->count; i++)
        {
            uint64_t p = iZm->root_primes->array[i];

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
        // Iterate through root primes, skipping 2, 3 and those that divide vx
        for (int i = k; i < iZm->root_primes->count; i++)
        {
            int p = iZm->root_primes->array[i];

            // Mark composites of p in x5 and x7
            bitmap_clear_steps_simd(vx_obj->x5, p, iZm_solve_for_xp_mpz(-1, p, vx, vx_obj->y), end_x);
            bitmap_clear_steps_simd(vx_obj->x7, p, iZm_solve_for_xp_mpz(1, p, vx, vx_obj->y), end_x);

            vx_obj->bit_ops += (2 * end_x) / p; // approximate number of bit operations
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

// Must not be used without prior deterministic sieving
static void vx_prob_sieve(VX_SEG *vx_obj)
{
    // If is_large_limit is false, no need for probabilistic testing
    if (vx_obj->is_large_limit == 0)
    {
        printf("vx_obj->is_large_limit is false, no need for probabilistic testing");
        return;
    }

    assert(vx_obj->x5 && vx_obj->x7 && "vx_obj bitmaps are NULL in vx_prob_sieve");
    assert(vx_obj->bit_ops > 0 && "Aborting vx_prob_sieve, deterministic sieving not performed");

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

        // test iZ(vx * y + x, 1) for primality
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
}

// streams primes as they are found to the output file
void vx_stream_file(IZM *iZm, VX_SEG *vx_obj, FILE *output)
{
    assert(iZm && "iZm is NULL in vx_stream_file");
    assert(vx_obj->x5 && vx_obj->x7 && "vx_obj bitmaps are NULL in vx_stream_file");
    assert(output && "output file is NULL in vx_stream_file");

    vx_det_sieve(iZm, vx_obj);

    // Initialize GMP reusable variables p, x_p
    mpz_t p, x_p;
    mpz_init(p);
    mpz_init(x_p);

    size_t prime_size = mpz_sizeinbase(vx_obj->yvx, 2) + 1;
    char buffer[prime_size];

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
            int is_prime = (vx_obj->is_large_limit) ? mpz_probab_prime_p(p, r) : 1;
            vx_obj->p_test_ops++;

            // if is_prime, increment count and output prime, else clear composite from x5
            if (is_prime)
            {
                vx_obj->p_count++;
                gmp_snprintf(buffer, sizeof(buffer), "%Zd ", p);
                fputs(buffer, output);
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
            int is_prime = (vx_obj->is_large_limit) ? mpz_probab_prime_p(p, r) : 1;
            vx_obj->p_test_ops++;

            if (is_prime)
            {
                vx_obj->p_count++;
                gmp_snprintf(buffer, sizeof(buffer), "%Zd ", p);
                fputs(buffer, output);
            }
            else
            {
                bitmap_clear_bit(vx_obj->x7, x); // Clear composite from x7
            }
        }
    }
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
 * @param iZm The IZM containing the reusable assets for the sieve process.
 * @param vx_obj The VX_SEG to be processed.
 * @param collect_p_gaps 0 only counts primes and doesn't store prime gaps,
 * other non-zero int values populates vx_obj->p_gaps with detected prime gaps.
 */
void vx_full_sieve(IZM *iZm, VX_SEG *vx_obj, int collect_p_gaps)
{
    assert(iZm != NULL && "iZm is NULL in vx_full_sieve");
    assert(vx_obj != NULL && "vx_obj is NULL in vx_full_sieve");

    // Perform deterministic sieving
    vx_det_sieve(iZm, vx_obj);

    // If is_large_limit is true, perform probabilistic primality tests
    if (vx_obj->is_large_limit)
        vx_prob_sieve(vx_obj);

    // If collect_p_gaps is true, populate the p_gaps array
    if (collect_p_gaps)
        vx_collect_p_gaps(vx_obj);
}

/**
 * @brief vx_write_file - Write a VX_SEG structure to a binary file.
 *
 * @description:
 * This function serializes the VX_SEG structure into a binary file. It writes the following data:
 *   - The length of the y string followed by the y string.
 *   - The count value indicating the number of elements in the p_gaps array.
 *   - The p_gaps array itself.
 *   - A SHA256 hash computed over the p_gaps array for data integrity, which is then written to the file.
 *
 * Parameters:
 * @param vx_obj: Pointer to a VX_SEG structure containing data to be written.
 * @param filename: The full path of the file to write to. If the filename does not include the
 *            ".vx6" extension, it is automatically appended.
 *
 * @return:
 *   - 1 on successful write,
 *   - 0 if any error occurs (e.g., invalid parameters, failure to open the file, or file write errors).
 */
int vx_write_file(VX_SEG *vx_obj, char *filename)
{
    if (vx_obj == NULL || filename == NULL)
        return 0;

    // check if filename ends with .vx, if not append it
    char f_buffer[256];
    strcpy(f_buffer, filename);
    if (strstr(f_buffer, VX_EXT) == NULL)
    {
        strcat(f_buffer, VX_EXT);
    }

    FILE *file = fopen(f_buffer, "wb");
    if (file == NULL)
    {
        log_error("Could not open file %s for writing", f_buffer);
        return 0;
    }

    // Write vx
    fwrite(&vx_obj->vx, sizeof(int), 1, file);

    // Write the length of the y string including null terminator
    size_t y_len = strlen(mpz_get_str(NULL, 10, vx_obj->y)) + 1;
    fwrite(&y_len, sizeof(size_t), 1, file);

    // Write the y string
    fwrite(mpz_get_str(NULL, 10, vx_obj->y), sizeof(char), y_len, file);

    // Write mr_rounds
    fwrite(&vx_obj->mr_rounds, sizeof(int), 1, file);

    // Write p_count
    fwrite(&vx_obj->p_count, sizeof(size_t), 1, file);

    // Write p_gaps array
    ui16_fwrite(vx_obj->p_gaps, file);

    fclose(file);
    return 1;
}

/**
 * vx_read_file - Read a VX_SEG structure from a binary file.
 *
 * @description:
 * This function reads the VX_SEG structure from a binary file by performing the following steps:
 *   - Reads the length of the y string and allocates memory for it, then reads the y string.
 *   - Reads the count value to determine the number of elements in the p_gaps array.
 *   - Reads the p_gaps array from the file.
 *   - Reads the previously stored SHA256 hash and computes a new hash on the read p_gaps array.
 *   - Compares the computed hash with the read hash to validate data integrity.
 *
 *  Parameters:
 * @param vx_obj: Pointer to a VX_SEG structure where the read data will be stored.
 * @param filename: The name (or path) of the file to read from. If the filename does not include the
 *            ".vx6" extension, it is automatically appended.
 *
 * @return:
 *   - 1 if the file is successfully read and the hash validation passes,
 *   - 0 if any error occurs (e.g., invalid parameters, file read errors, memory allocation failure,
 *         or hash mismatch).
 */
VX_SEG *vx_read_file(char *filename)
{
    if (filename == NULL)
        return NULL;

    char f_buffer[256];
    strcpy(f_buffer, filename);
    if (strstr(f_buffer, VX_EXT) == NULL)
    {
        strcat(f_buffer, VX_EXT);
    }

    FILE *file = fopen(f_buffer, "rb");
    if (file == NULL)
    {
        printf("Could not open file %s for reading\n", f_buffer);
        return NULL;
    }

    // Read vx
    int vx;
    if (fread(&vx, sizeof(int), 1, file) != 1)
    {
        log_error("Failed to read vx from file: %s", filename);
        fclose(file);
        return NULL;
    }

    // Read the length of the y string and allocate memory
    size_t y_len;
    fread(&y_len, sizeof(size_t), 1, file);

    char y_buffer[y_len];
    if (fread(&y_buffer, sizeof(char), y_len, file) != y_len)
    {
        log_error("Failed to read y string from file: %s", filename);
        fclose(file);
        return NULL;
    }
    // Initialize VX_SEG object with read y string
    VX_SEG *vx_obj = vx_init(vx, y_buffer, 0);
    if (vx_obj == NULL)
    {
        log_error("vx_init failed in vx_read_file\n");
        fclose(file);
        return NULL;
    }

    // Read mr_rounds
    if (fread(&vx_obj->mr_rounds, sizeof(int), 1, file) != 1)
    {
        log_error("Failed to read mr_rounds from file: %s", filename);
        fclose(file);
        vx_free(&vx_obj);
        return NULL;
    }

    // Read p_count
    if (fread(&vx_obj->p_count, sizeof(size_t), 1, file) != 1)
    {
        log_error("Failed to read p_count from file: %s", filename);
        fclose(file);
        vx_free(&vx_obj);
        return NULL;
    }

    // Read p_gaps array
    vx_obj->p_gaps = ui16_fread(file);

    fclose(file);
    return vx_obj;
}
