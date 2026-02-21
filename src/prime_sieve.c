/**
 * @file prime_sieve.c
 * @brief Implementations of classical and SiZ-family sieve algorithms.
 *
 * This file contains the implementations of various prime sieving algorithms, including
 * classical algorithms (SoE/SSoE/SoEu/SoS/SoA) as well as SiZ-family algorithms
 * (SiZ/SiZm/SiZm_vy). All functions are single-threaded, take an upper limit `n` and
 * return a pointer to a UI64_ARRAY containing the prime numbers up to `n`.
 *
 * @ingroup iz_api
 */

#include <iZ_api.h>

// ==================================================================
// * Internal helper macros:
// ==================================================================

/** Maximum supported sieve limit for standard entry points (10^12). */
#define N_LIMIT (1000000000000ULL)

/** @brief Assert that input n is within the valid range for sieve functions. */
#define ASSERT_LIMIT(n) assert((n) > 10 && (n) <= N_LIMIT && "Input must be in the range (10, 10^12).")

/** @brief Pi(n) is an approximation of the number of primes up to n, used for initial array sizing. */
#define Pi(n) ((n / log(n)))

// =========================================================
// * Classic Sieve Algorithms
// =========================================================

/**
 * @brief A helper function to process the bitmap for the Sieve of Eratosthenes and collect primes.
 * @param primes Output prime array.
 * @param sieve_bitmap Candidate bitmap.
 * @param n Inclusive numeric upper bound represented in @p sieve_bitmap.
 */
static void process_N_bitmap(UI64_ARRAY *primes, BITMAP *sieve_bitmap, uint64_t n)
{
    ui64_push(primes, 2); // Add 2 to primes to focus on odd candidates
    uint64_t n_sqrt = sqrt(n);

    // Sieve logic:
    // Iterate through odd numbers i starting from 3,
    // check if i is marked as prime in the bitmap,
    // if so, collect i as a prime and mark its odd multiples if i <= sqrt(n).
    for (uint64_t i = 3; i <= n; i += 2)
    {
        if (bitmap_get_bit(sieve_bitmap, i))
        {
            ui64_push(primes, i);
            if (i <= n_sqrt)
                bitmap_clear_steps_simd(sieve_bitmap, 2 * i, i * i, n + 1);
        }
    }
}

/**
 * @ingroup iz_api
 * @brief Optimized Sieve of Eratosthenes algorithm to find all primes up to n.
 *
 * Description:
 * This function applies common speedups to the Sieve of Eratosthenes algorithm.
 * It works the same way as the classic sieve but skips checking or marking even
 * numbers and starts with 3, incrementing by 2, it also starts the sieve from p*p.
 *
 * @param n The upper limit to find primes.
 * @return
 *      - A pointer to a UI64_ARRAY structure containing the list of prime numbers up to n,
 *      - NULL if memory allocation fails or if n is less than 10.
 */
UI64_ARRAY *SoE(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    // Initialize the primes object with an estimated capacity
    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4); // 40% over-estimation to avoid reallocs
    assert(primes && "Memory allocation failed for primes array.");

    // Create a bitmap to mark prime numbers
    BITMAP *sieve = bitmap_init(n + 1, 1);
    if (!sieve)
    {
        ui64_free(&primes);
        return NULL;
    }

    // Sieve logic
    process_N_bitmap(primes, sieve, n);

    // cleanup and finalize
    bitmap_free(&sieve);        // free bitmap
    ui64_resize_to_fit(primes); // Trim excess memory in primes array

    return primes;
}

/**
 * @ingroup iz_api
 * @brief Segmented Sieve of Eratosthenes algorithm to find all primes up to n.
 *
 * Description:
 * This function implements the Segmented Sieve of Eratosthenes algorithm to find all prime numbers
 * up to a given limit n. It divides the range into segments and uses a bitmap to mark prime numbers
 * in each segment. The function returns a pointer to a UI64_ARRAY structure containing the list of
 * prime numbers found. The function also handles memory allocation and resizing of the primes array
 * as needed.
 *
 * @param n The upper limit to find primes.
 * @return
 *      - A pointer to the UI64_ARRAY structure containing the list of primes up to n,
 *      - NULL if memory allocation fails or if n is less than 1000.
 */
UI64_ARRAY *SSoE(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    // Initialize UI64_ARRAY with an estimated capacity
    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4); // 40% over-estimation to avoid reallocs
    assert(primes && "Memory allocation failed for primes array.");

    // Define the segment size; can be tuned based on memory constraints
    uint64_t segment_size = (uint64_t)sqrt(n);

    // * Step 1: Sieve small primes up to sqrt(n) using the traditional sieve
    BITMAP *sieve = bitmap_init(segment_size + 8, 1);
    if (!sieve)
    {
        ui64_free(&primes);
        return NULL;
    }

    // process first segment to collect root primes
    process_N_bitmap(primes, sieve, segment_size);

    // * Step 2: Segmented sieve
    uint64_t low = segment_size + 1;
    uint64_t high = low + segment_size - 1;

    // Iterate over segments
    while (low <= n)
    {
        bitmap_set_all(sieve); // Reset segment bitmap
        uint64_t root_limit = sqrt(high);

        // Sieve the current segment using primes <= sqrt(high)
        for (int i = 1; i < primes->count; i++) // skip 2
        {
            uint64_t p = primes->array[i];
            if (p > root_limit)
                break;

            // Find the minimum number in [low, high] that is a multiple of p
            uint64_t start = (low / p) * p;
            start += (start < low) ? p : 0;    // ensure start >= low
            start += (start % 2 == 0) ? p : 0; // ensure start is odd
            start = MAX(p * p, start);         // start from p^2 or higher

            // Mark multiples of p within the segment
            bitmap_clear_steps_simd(sieve, 2 * p, start - low, high - low + 1);
        }

        // Collect primes from the current segment
        uint64_t i = (low % 2 == 0) ? low + 1 : low;
        for (; i <= high; i += 2) // skip even numbers
        {
            if (bitmap_get_bit(sieve, i - low))
                ui64_push(primes, i);
        }

        // Move to the next segment
        low += segment_size;
        high += segment_size;
        if (high > n)
            high = n;
    }

    // * Step 3: Cleanup and finalize
    bitmap_free(&sieve);        // free bitmap
    ui64_resize_to_fit(primes); // Trim excess memory in primes array

    return primes;
}

/**
 * @ingroup iz_api
 * @brief Sieve of Euler: Generates a list of prime numbers up to a given limit using the Euler Sieve algorithm.
 *
 * Description:
 * This function uses the Euler Sieve algorithm to find all prime numbers up to a specified limit `n`.
 * It marks each composite only once, allowing for a more efficient sieve process. It initializes a bitmap
 * to track prime numbers and iterates through the numbers, marking distinct multiples of each prime found.
 * The function also handles memory allocation and resizing of the primes array as needed.
 *
 * @param n The upper limit up to which prime numbers are to be found.
 * @return
 *      - UI64_ARRAY* A pointer to the UI64_ARRAY structure containing the list of primes up to n.
 *      - NULL if memory allocation fails or if n is less than 10.
 */
UI64_ARRAY *SoEu(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    // initialization
    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4); // 40% over-estimation to avoid reallocs
    assert(primes && "Memory allocation failed for primes array.");

    BITMAP *sieve = bitmap_init(n + 1, 1);
    if (sieve == NULL)
    {
        ui64_free(&primes);
        return NULL;
    }

    // starting the prime list with 2 to skip reading even numbers
    ui64_push(primes, 2);

    // sieve logic: iterate through odd numbers, marking composites by clearing multiples of primes
    for (uint64_t i = 3; i <= n; i += 2)
    {
        if (bitmap_get_bit(sieve, i))
            ui64_push(primes, i);

        // Mark multiples of the current prime
        for (int j = 1; j < primes->count; ++j)
        {
            uint64_t p = primes->array[j];

            if (p * i > n)
                break;

            bitmap_clear_bit(sieve, p * i);

            if (i % p == 0)
                break;
        }
    }

    // cleanup
    bitmap_free(&sieve);

    // Resize primes array to fit the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

/**
 * @ingroup iz_api
 * @brief Sieve of Sundaram: Generates a list of prime numbers up to a given limit using the Sieve of Sundaram algorithm.
 *
 * Description:
 * This function implements the Sieve of Sundaram algorithm to find all prime numbers
 * up to a specified limit `n`. The Sieve of Sundaram works by eliminating numbers
 * of the form `i + j + 2ij` from a list of integers from 1 to `k = (n-1)/2`.
 * If an integer `x` in this range is not eliminated, then `2x+1` is a prime number.
 * The prime number 2 is handled as a special case, as the sieve only generates odd primes.
 *
 * @param n The upper limit (inclusive) up to which prime numbers are to be found.
 * @return
 *      - UI64_ARRAY* A pointer to the UI64_ARRAY structure containing the list of primes up to n.
 *      - NULL if memory allocation fails or if n is less than 10.
 */
UI64_ARRAY *SoS(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    // Calculate k as the odd numbers up to n.
    uint64_t k = (n - 1) / 2 + 1;

    // Initialize the primes object with an estimated capacity
    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4); // 40% over-estimation to avoid reallocs
    assert(primes && "Memory allocation failed for primes array.");

    ui64_push(primes, 2);

    // Create a bitmap with size k+1.
    BITMAP *sieve = bitmap_init(k + 8, 1);
    if (sieve == NULL)
    {
        ui64_free(&primes);
        return NULL;
    }

    uint64_t n_sqrt = sqrt(n) + 1;

    // iterate through odd numbers
    for (uint64_t i = 1; i < k; ++i)
    {
        if (bitmap_get_bit(sieve, i))
        {
            uint64_t p = 2 * i + 1;
            ui64_push(primes, p);
            if (p < n_sqrt)
            {
                // First composite mark xp in the bitmap is given by:
                // xp = 2 * i^2 + 2 * i = p * i + i, corresponding to p^2 in the odd set
                uint64_t xp = p * i + i;
                // Mark composites of p additively in the bitmap using step size p
                bitmap_clear_steps_simd(sieve, p, xp, k);
            }
        }
    }

    // Cleanup
    bitmap_free(&sieve);

    // Resize primes array to fit the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

/**
 * @ingroup iz_api
 * @brief Sieve of Atkin: Generates a list of prime numbers up to a given limit using the Sieve of Atkin algorithm.
 *
 * Description:
 * This function implements the Sieve of Atkin, a modern algorithm to find all prime numbers up to a specified integer `n`.
 * It initializes a bitmap to mark potential primes and applies the Atkin conditions to identify primes.
 * The function also marks odd multiples of squares of primes as non-prime, then collects the remaining unmarked numbers as primes.
 *
 * @param n The upper limit (inclusive) up to which prime numbers are to be found.
 * @return
 *      - UI64_ARRAY* A pointer to the UI64_ARRAY structure containing the list of primes up to n.
 *      - NULL if memory allocation fails or if n is less than 10.
 */
UI64_ARRAY *SoA(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4); // 40% over-estimation to avoid reallocs
    assert(primes && "Memory allocation failed for primes array.");

    // Create a bitmap to mark potential primes
    BITMAP *sieve = bitmap_init(n + 1, 0);
    if (sieve == NULL)
    {
        ui64_free(&primes);
        return NULL;
    }

    // Add 2 and 3 to primes
    ui64_push(primes, 2);
    ui64_push(primes, 3);

    // 1. Mark potential primes in the bitmap using the Atkin conditions
    // Condition 1: for all 4x^2 + y^2 <= n
    for (uint64_t x = 1; 4 * x * x < n; x++)
    {
        uint64_t a = 4 * x * x;
        for (uint64_t y = 1; a + y * y <= n; ++y)
        {
            uint64_t b = a + y * y;
            // if 4x^2 + y^2 ≡ 1 or 5 (mod 12), flip the bit
            if (b % 12 == 1 || b % 12 == 5)
                bitmap_flip_bit(sieve, b);
        }
    }

    // Condition 2: for all 3x^2 + y^2 <= n
    for (uint64_t x = 1; 3 * x * x < n; x++)
    {
        uint64_t a = 3 * x * x;
        for (uint64_t y = 1; a + y * y <= n; ++y)
        {
            uint64_t b = a + y * y;
            // if 3x^2 + y^2 ≡ 7 (mod 12), flip the bit
            if (b % 12 == 7)
                bitmap_flip_bit(sieve, b);
        }
    }

    // Condition 3: for all 3x^2 - y^2 <= n and x > y
    for (uint64_t x = 1; 2 * x * x < n; x++) // Approximation for loop bound
    {
        uint64_t a = 3 * x * x;
        for (uint64_t y = x - 1; y > 0; y--)
        {
            uint64_t b = a - y * y;
            if (b > n)
                break; // b increases as y decreases

            // if 3x^2 - y^2 ≡ 11 (mod 12), flip the bit
            if (b % 12 == 11)
                bitmap_flip_bit(sieve, b);
        }
    }

    // 2. Remove remaining composites by sieving out multiples of squares of root primes
    uint64_t n_sqrt = sqrt(n);
    for (uint64_t p = 5; p <= n_sqrt; p += 2)
    {
        if (bitmap_get_bit(sieve, p))
        {
            // Mark odd multiples of p^2 as non-prime
            // starting at p^2 with step size 2p^2
            bitmap_clear_steps_simd(sieve, 2 * p * p, p * p, n + 1);
        }
    }

    // 3. Collect primes from the bitmap
    for (uint64_t p = 5; p <= n; p += 2)
    {
        if (bitmap_get_bit(sieve, p))
            ui64_push(primes, p);
    }

    // cleanup
    bitmap_free(&sieve);

    // Resize primes array to fit the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

// =========================================================
// * Sieve-iZ Algorithms
// =========================================================

/**
 * @ingroup iz_api
 * @brief Classic Sieve-iZ algorithm for prime generation up to n.
 *
 * The Sieve-iZ algorithm operates in the iZ index space of numbers of the
 * form 6x ± 1, excluding all multiples of 2 and 3. It maintains two bitmaps,
 * x5 and x7, representing candidates of the form 6x − 1 (iZ−) and 6x + 1
 * (iZ+), respectively. For each index x whose bit remains set, the
 * corresponding candidate is treated as prime and its composites are marked
 * using the Xp identity in the iZ space.
 *
 * This provides a wheel factorization of modulo 6, reducing the number of
 * candidates by approximately a factor of 3 compared to a classical sieve
 * over all integers. After sieving, any remaining set bits correspond to
 * primes of the form 6x ± 1, which are mapped back to their integer values
 * and appended to the output array alongside the primes 2 and 3.
 *
 * @param n Upper bound (inclusive) for prime generation. Must satisfy
 *          10 < n <= pow(10, 12).
 * @return Pointer to a UI64_ARRAY containing all primes <= n on success,
 *         or NULL if allocation fails.
 */
UI64_ARRAY *SiZ(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    // Initialize primes object with enough initial estimation
    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4); // 40% over-estimation to avoid reallocs
    assert(primes && "Memory allocation failed for primes array.");

    // Add 2, 3 to primes
    ui64_push(primes, 2);
    ui64_push(primes, 3);

    // Calculate x_n, max x value in iZ space for given n
    uint64_t x_n = n / 6 + 1;

    // Create bitmap X-Arrays x5, x7, each of size x_n + 1 bits
    BITMAP *x5 = bitmap_init(x_n + 1, 1);
    BITMAP *x7 = bitmap_init(x_n + 1, 1);

    // Memory allocation failed, check logs
    if (!x5 || !x7)
    {
        ui64_free(&primes);
        return NULL;
    }

    // Sieve logic
    process_iZ_bitmaps(primes, x5, x7, x_n);

    // Cleanup: free memory of x5, x7
    bitmap_free(&x5);
    bitmap_free(&x7);

    // Handle edge case: if last prime > n, remove it
    if (primes->array[primes->count - 1] > n)
        ui64_pop(primes);

    // Trim unused memory in primes object
    ui64_resize_to_fit(primes);

    return primes;
}

/**
 * @ingroup iz_api
 * @brief Segmented Sieve-iZm algorithm for prime generation up to n.
 *
 * Description:
 * The Sieve-iZm algorithm is a segmented, cache-aware variant of the Sieve-iZ
 * that operates in the iZ index space of numbers of the form 6x ± 1. It
 * partitions the search interval into segments of length vx, where vx is a
 * product of small primes, and constructs a pre-sieved base segment using
 * those small primes once at initialization time.
 *
 * For each segment, Sieve-iZm clones the base bitmaps and only marks
 * composites of the remaining root primes that do not divide vx. This
 * amortizes the cost of small-prime sieving across all segments and keeps the
 * working set small and highly cache-resident. After marking, any remaining
 * set bits correspond to primes of the form 6x ± 1 within that segment, which
 * are then mapped back to their integer values and appended to the output
 * array.
 *
 * Compared to the basic Sieve-iZ, Sieve-iZm achieves the same
 * O(n log log n) time complexity with effectively constant auxiliary memory
 * (aside from the output) and significantly reduced constant factors, making
 * it well-suited for enumerating primes up to large bounds.
 *
 * @param n Upper bound (inclusive) for prime generation. Must satisfy
 *          10 < n <= pow(10, 12).
 * @return Pointer to a UI64_ARRAY containing all primes <= n on success,
 *         or NULL on allocation or initialization failure.
 */
UI64_ARRAY *SiZm(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    // if n < 10000, return SiZ(n), doesn't worth segmenting
    if (n < 10000)
        return SiZ(n);

    // * 1. Initialization:
    // Initialize primes array with enough capacity to avoid reallocs
    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4); // 40% over-estimation to avoid reallocs
    assert(primes && "Memory allocation failed for primes array in SiZm.");

    // Compute vx wheel size that fits in L2 cache
    int vx = compute_l2_vx(n);

    // Initialize and construct base bitmaps for iZm/vx
    BITMAP *base_x5 = bitmap_init(vx + 8, 1);
    BITMAP *base_x7 = bitmap_init(vx + 8, 1);
    if (!base_x5 || !base_x7)
    {
        ui64_free(&primes);
        return NULL;
    }
    iZm_construct_vx_base(vx, base_x5, base_x7);

    // Add the pre-sieved k primes to primes array
    const uint64_t base_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    int k = 0;
    while ((6 * vx) % base_primes[k] == 0)
        ui64_push(primes, base_primes[k++]);

    // * 2. Process first segment (y = 0) to collect root primes:
    uint64_t x_n = n / 6 + 1; // max x value up to n
    // uint64_t root_limit = sqrt(6 * x_limit) + 1;
    // Initialize active sieve bitmaps from base
    BITMAP *x5 = bitmap_clone(base_x5);
    BITMAP *x7 = bitmap_clone(base_x7);
    if (!x5 || !x7)
    {
        ui64_free(&primes);
        bitmap_free(&base_x5);
        bitmap_free(&base_x7);
        return NULL;
    }
    process_iZ_bitmaps(primes, x5, x7, vx + 1);

    // * 3. Process remaining segments (y >= 1) to collect primes:
    int y_limit = x_n / vx; // number of full segments to process
    uint64_t yvx = vx;      // current base value (y * vx)
    for (int y = 1; y <= y_limit; y++)
    {
        // * a. Reset active bitmaps to base state
        memcpy(x5->data, base_x5->data, x5->byte_size);
        memcpy(x7->data, base_x7->data, x7->byte_size);

        int x_limit = (y < y_limit) ? vx : (int)(x_n % (uint64_t)vx); // local x limit adjusted for last segment
        uint64_t root_limit = sqrt(6 * (yvx + x_limit)) + 1; // local root limit for current segment

        // * b. Mark composites of root primes in current segment
        for (int i = k; i < primes->count; i++)
        {
            uint64_t p = primes->array[i];
            if (p > root_limit)
                break;

            // mark composites of p in current segment
            bitmap_clear_steps_simd(x5, p, iZm_solve_for_x0(-1, p, vx, y), x_limit);
            bitmap_clear_steps_simd(x7, p, iZm_solve_for_x0(1, p, vx, y), x_limit);
        }

        // * c. Collect unmarked indices as primes in current segment
        for (int x = 2; x <= x_limit; x++)
        {
            if (bitmap_get_bit(x5, x)) // i.e. iZ- prime
                ui64_push(primes, iZ(yvx + x, -1));

            if (bitmap_get_bit(x7, x)) // i.e. iZ+ prime
                ui64_push(primes, iZ(yvx + x, 1));
        }

        yvx += vx; // advance yvx for next segment
    }

    // * 4. Clean up and finalize
    bitmap_free(&x5);
    bitmap_free(&x7);
    bitmap_free(&base_x5);
    bitmap_free(&base_x7);

    // Handle edge case: if last prime > n, remove it
    if (primes->array[primes->count - 1] > n)
        ui64_pop(primes);

    ui64_resize_to_fit(primes); // Trim excess memory in primes array
    return primes;
}

/**
 * @ingroup iz_api
 * @brief Segmented Sieve-iZm with vertical (vy) traversal order.
 *
 * This variant prioritizes throughput and returns primes in non-sorted order.
 *
 * @param n Inclusive upper bound for prime generation.
 * @return Heap-allocated list of primes <= @p n, or NULL on failure.
 */
UI64_ARRAY *SiZm_vy(uint64_t n)
{
    ASSERT_LIMIT(n); // Validate input limit

    // if n is less than 10000, return SiZ(n)
    if (n < 10000)
        return SiZ(n);

    // * 1. Initialization:
    // Initialize primes array with enough capacity to avoid reallocs
    UI64_ARRAY *primes = ui64_init(Pi(n) * 1.4);
    assert(primes && "Memory allocation failed for primes array in SiZm.");

    uint64_t x_n = n / 6 + 1; // iZ limit for n
    uint64_t root_limit = sqrt(n) + 1;

    get_root_primes(primes, root_limit);
    int root_count = primes->count;

    int k = 4; // pointing at 11 in root_primes
    int vx = 35;
    if (n >= pow(10, 9))
    {
        vx *= 11;
        k++;
    }
    if (n >= pow(10, 11))
    {
        vx *= 13;
        k++;
    }

    int vy = x_n / vx;

    BITMAP *sieve = bitmap_init(vy + 8, 1);

    // * 2. Sieve logic: Process iZm's columns as segments
    for (int x = 2; x <= vx; x++)
    {
        // Handle iZ- case:
        // crucial, ensure iZ(x, -1) is coprime to vx before sieving
        if (gcd(iZ(x, -1), vx) == 1)
        {
            // * a. reset sieve bitmap for new segment
            bitmap_set_all(sieve); // set all bits

            // * b. mark composites of root primes in sieve
            for (int i = k; i < root_count; i++)
            {
                uint64_t p = primes->array[i];
                int64_t y_0 = iZm_solve_for_y0(-1, p, vx, x);
                bitmap_clear_steps_simd(sieve, p, y_0, vy);
            }

            // * c. collect primes from sieve up to y = vy-1
            for (int y = 0; y < vy; y++)
            {
                if (bitmap_get_bit(sieve, y))
                    ui64_push(primes, iZ(y * vx + x, -1));
            }
            // handle partial last row where y = vy (check if p < n before pushing)
            if (bitmap_get_bit(sieve, vy))
            {
                uint64_t p = iZ(vy * vx + x, -1);
                if (p < n)
                    ui64_push(primes, p);
            }
        }

        // Handle iZ+ case similarly:
        if (gcd(iZ(x, 1), vx) == 1)
        {
            bitmap_set_all(sieve); // reset sieve bitmap for new segment
            for (int i = k; i < root_count; i++)
            {
                uint64_t p = primes->array[i];
                int64_t y_0 = iZm_solve_for_y0(1, p, vx, x);
                bitmap_clear_steps_simd(sieve, p, y_0, vy);
            }

            for (int y = 0; y < vy; y++)
            {
                if (bitmap_get_bit(sieve, y))
                    ui64_push(primes, iZ(y * vx + x, 1));
            }
            if (bitmap_get_bit(sieve, vy))
            {
                uint64_t p = iZ(vy * vx + x, 1);
                if (p < n)
                    ui64_push(primes, p);
            }
        }
    }

    // * 3. Clean up and finalize
    bitmap_free(&sieve);
    ui64_resize_to_fit(primes); // Trim excess memory in primes array

    primes->ordered = 0; // Mark the array as unordered
    return primes;
}
