/**
 * @file prime_sieve.c
 * @brief Sieve-iZ family of algorithms for prime generation and counting.
 *
 * This module implements several related algorithms based on the iZ
 * framework, which operates in the iZ index space of numbers of the form
 * 6x ± 1. By excluding multiples of 2 and 3 and working directly in this
 * reduced space, these algorithms achieve substantial constant-factor
 * improvements over classical sieves while preserving the usual
 * O(n log log n) time complexity.
 *
 * The following entry points are provided:
 * * Classic Sieve Algorithms:
 * - @b SoE: Optimized Sieve of Eratosthenes
 * - @b SSoE: Segmented Sieve of Eratosthenes
 * - @b Sieve_Euler: Sieve of Euler
 * - @b Sieve_Sundaram: Sieve of Sundaram
 * - @b Sieve_Atkin: Sieve of Atkin
 * * Sieve-iZ Algorithms:
 * - @b SiZ: Classic Sieve-iZ
 * - @b SiZm: Segmented Sieve-iZm
 * * Sieve-iZ range variants:
 * - @b SiZ_stream: Range-based variant that counts, and optionally
 *   streams, primes within an arbitrary numeric interval [Zs, Ze] using the
 *   VX module.
 * - @b SiZ_count: Multi-process range counter that partitions the
 *   iZ index space across worker processes, aggregating per-process counts to
 *   obtain the total number of primes in [Zs, Ze].
 */

#include <iZ_api.h>

/**
 * @brief Estimate the number of primes up to n using the prime number theorem.
 *
 * @param n The input value.
 * @return uint64_t The result of the prime counting function (n/ln(n)) * 1.4.
 */
static uint64_t est_pi_n(int64_t n)
{
    return (n / log(n)) * 1.4; // 40% over-estimation to avoid reallocs
}

// =========================================================
// * Classic Sieve Algorithms: Implementations
// =========================================================

/**
 * @brief Optimized Sieve of Eratosthenes algorithm to find all primes up to n.
 *
 * @description:
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
    // Bound check:
    assert(n > 10 && n <= pow(10, 12) && "Input must be in the range (10, 10^12).");

    // Initialize the primes object with an estimated capacity
    UI64_ARRAY *primes = ui64_init(est_pi_n(n));
    assert(primes && "Memory allocation failed for primes array.");

    // Create a bitmap to mark prime numbers
    BITMAP *n_bits = bitmap_init(n + 1, 1);
    if (n_bits == NULL)
    {
        ui64_free(&primes);
        return NULL;
    }

    uint64_t n_sqrt = sqrt(n);

    // Add 2, the only even prime so we can skip even numbers
    ui64_push(primes, 2);

    // Sieve logic: collect unmarked primes and mark their multiples if p <= sqrt(n),
    // skipping even numbers
    for (uint64_t p = 3; p <= n; p += 2)
    {
        if (bitmap_get_bit(n_bits, p))
        {
            ui64_push(primes, p);
            if (p <= n_sqrt)
            {
                bitmap_clear_steps_simd(n_bits, 2 * p, p * p, n + 1);
            }
        }
    }

    bitmap_free(&n_bits);

    // Resize primes array to fit the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

/**
 * @brief Segmented Sieve of Eratosthenes algorithm to find all primes up to n.
 *
 * @description:
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
    // Bound check:
    assert(n > 10 && n <= pow(10, 12) && "Input must be in the range (10, 10^12).");

    // Initialize UI64_ARRAY with an estimated capacity
    UI64_ARRAY *primes = ui64_init(est_pi_n(n));
    assert(primes && "Memory allocation failed for primes array.");

    // Define the segment size; can be tuned based on memory constraints
    uint64_t segment_size = (uint64_t)sqrt(n);

    // Step 1: Sieve small primes up to sqrt(n) using the traditional sieve
    BITMAP *n_bits = bitmap_init(segment_size + 1, 1); // +1 to include segment_size
    if (n_bits == NULL)
    {
        ui64_free(&primes);
        return NULL;
    }

    primes->array[primes->count++] = 2; // Add 2 as the first prime

    // Sieve odd numbers starting from 3 up to segment_size
    for (uint64_t p = 3; p <= segment_size; p += 2)
    {
        if (bitmap_get_bit(n_bits, p))
        {
            ui64_push(primes, p);

            // Start marking multiples of p from p*p within the small_bits
            for (uint64_t multiple = p * p; multiple <= segment_size; multiple += 2 * p)
                bitmap_clear_bit(n_bits, multiple);
        }
    }

    // Step 2: Segmented sieve
    uint64_t low = segment_size + 1;
    uint64_t high = low + segment_size - 1;
    if (high > n)
        high = n;

    // Iterate over segments
    while (low <= n)
    {
        bitmap_set_all(n_bits);

        // Sieve the current segment using the small primes
        for (int i = 0; i < primes->count; i++)
        {
            uint64_t p = primes->array[i];
            if (p * p > high)
                break;

            // Find the minimum number in [low, high] that is a multiple of p
            uint64_t start = (low / p) * p;
            if (start < low)
                start += p;
            if (start < p * p)
                start = p * p;

            // Mark multiples of p within the segment
            for (uint64_t j = start; j <= high; j += p)
            {
                // Skip even multiples
                if (j % 2 == 0)
                    continue;

                size_t index = j - low;
                bitmap_clear_bit(n_bits, index);
            }
        }

        // Collect primes from the current segment
        for (uint64_t i = low; i <= high; i++)
        {
            // Skip even numbers
            if (i % 2 == 0)
                continue;

            if (bitmap_get_bit(n_bits, i - low))
                ui64_push(primes, i);
        }

        // Move to the next segment
        low = high + 1;
        high = low + segment_size - 1;
        if (high > n)
            high = n;
    }

    // Step 3: Finalize
    // Trim the primes array to the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

/**
 * @brief Sieve of Euler: Generates a list of prime numbers up to a given limit using the Euler Sieve algorithm.
 *
 * @description:
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
    // Bound check:
    assert(n > 10 && n <= pow(10, 12) && "Input must be in the range (10, 10^12).");

    // initialization
    UI64_ARRAY *primes = ui64_init(est_pi_n(n));
    assert(primes && "Memory allocation failed for primes array.");

    BITMAP *n_bits = bitmap_init(n + 1, 1);
    if (n_bits == NULL)
    {
        ui64_free(&primes);
        return NULL;
    }

    // starting the prime list with 2 to skip reading even numbers
    ui64_push(primes, 2);

    // sieve logic: iterate through odd numbers, marking composites by clearing multiples of primes
    for (uint64_t i = 3; i <= n; i += 2)
    {
        if (bitmap_get_bit(n_bits, i))
            ui64_push(primes, i);

        // Mark multiples of the current prime
        for (int j = 1; j < primes->count; ++j)
        {
            uint64_t p = primes->array[j];

            if (p * i > n)
                break;

            bitmap_clear_bit(n_bits, p * i);

            if (i % p == 0)
                break;
        }
    }

    // cleanup
    bitmap_free(&n_bits);

    // Resize primes array to fit the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

/**
 * @brief Sieve of Sundaram: Generates a list of prime numbers up to a given limit using the Sieve of Sundaram algorithm.
 *
 * @description:
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
    // Bound check:
    assert(n > 10 && n <= pow(10, 12) && "Input must be in the range (10, 10^12).");

    // Calculate k as the odd numbers up to n.
    uint64_t k = (n - 1) / 2 + 1;

    // Initialize the primes object with an estimated capacity
    UI64_ARRAY *primes = ui64_init(est_pi_n(n));
    assert(primes && "Memory allocation failed for primes array.");

    ui64_push(primes, 2);

    // Create a bitmap with size k+1.
    BITMAP *k_bits = bitmap_init(k + 1, 1);
    if (k_bits == NULL)
    {
        ui64_free(&primes);
        return NULL;
    }

    uint64_t n_sqrt = sqrt(n) + 1;

    for (uint64_t i = 1; i < k; ++i)
    {
        if (bitmap_get_bit(k_bits, i))
        {
            uint64_t p = 2 * i + 1;
            ui64_push(primes, p);
            if (p < n_sqrt)
            {
                // Smallest composite mark of p (when j=i) is 2i + 2i^2
                uint64_t x = 2 * i * i + 2 * i;
                // Mark composites of p in the bitmap
                bitmap_clear_steps_simd(k_bits, p, x, k);
            }
        }
    }

    // Cleanup
    bitmap_free(&k_bits);

    // Handle edge case: if last prime > n, remove it
    if (primes->array[primes->count - 1] > n)
        primes->count--;

    // Resize primes array to fit the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

/**
 * @brief Sieve of Atkin: Generates a list of prime numbers up to a given limit using the Sieve of Atkin algorithm.
 *
 * @description:
 * This function implements the Sieve of Atkin, an efficient algorithm to find all prime numbers up to a specified integer `n`.
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
    // Bound check:
    assert(n > 10 && n <= pow(10, 12) && "Input must be in the range (10, 10^12).");

    UI64_ARRAY *primes = ui64_init(est_pi_n(n));
    assert(primes && "Memory allocation failed for primes array.");

    // Create a bitmap to mark potential primes
    BITMAP *n_bits = bitmap_init(n + 1, 0);
    if (n_bits == NULL)
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
                bitmap_flip_bit(n_bits, b);
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
                bitmap_flip_bit(n_bits, b);
        }
    }

    // Condition 3: for all 3x^2 - y^2 <= n and x > y
    for (uint64_t x = 1; 2 * x * x < n; x++) // Approximation for loop bound
    {
        uint64_t a = 3 * x * x;
        for (uint64_t y = x - 1; y >= 1; y--)
        {
            uint64_t b = a - y * y;
            if (b > n)
                break; // b increases as y decreases

            // if 3x^2 - y^2 ≡ 11 (mod 12), flip the bit
            if (b % 12 == 11)
                bitmap_flip_bit(n_bits, b);
        }
    }

    // 2. Remove remaining composites by sieving out multiples of squares of root primes
    uint64_t n_sqrt = sqrt(n);
    for (uint64_t p = 5; p <= n_sqrt; p += 2)
    {
        if (bitmap_get_bit(n_bits, p))
        {
            // Mark odd multiples of p^2 as non-prime
            // starting at p^2 with step size 2p^2
            bitmap_clear_steps_simd(n_bits, 2 * p * p, p * p, n + 1);
        }
    }

    // 3. Collect primes from the bitmap
    for (uint64_t p = 5; p <= n; p += 2)
    {
        if (bitmap_get_bit(n_bits, p))
            ui64_push(primes, p);
    }

    // cleanup
    bitmap_free(&n_bits);

    // Resize primes array to fit the exact number of primes found
    ui64_resize_to_fit(primes);

    return primes;
}

// =========================================================
// * Sieve-iZ Algorithms: Implementations
// =========================================================

/**
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
    // Sanity check:
    assert(n > 10 && n <= pow(10, 12) && "Input must be in the range (10, 10^12).");

    // Initialize primes object with enough initial estimation
    UI64_ARRAY *primes = ui64_init(est_pi_n(n));
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

    // Calculate n_sqrt: the upper bound for root primes
    uint64_t n_sqrt = sqrt(n) + 1;

    // Iterate through x values in range 0 < x < x_n
    for (uint64_t x = 1; x < x_n; x++)
    {
        // if x5[x], implying it's iZ- prime
        if (bitmap_get_bit(x5, x))
        {
            uint64_t p = iZ(x, -1); // compute p = iZ(x, -1)
            ui64_push(primes, p);   // add p to primes

            // if p is root prime, mark its multiples in x5, x7
            if (p < n_sqrt)
            {
                bitmap_clear_steps_simd(x5, p, p * x + x, x_n);
                bitmap_clear_steps_simd(x7, p, p * x - x, x_n);
            }
        }

        // Do the same if x7[x], inverting the xp signs
        if (bitmap_get_bit(x7, x))
        {
            uint64_t p = iZ(x, 1);
            ui64_push(primes, p);

            if (p < n_sqrt)
            {
                bitmap_clear_steps_simd(x5, p, p * x - x, x_n);
                bitmap_clear_steps_simd(x7, p, p * x + x, x_n);
            }
        }
    }

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
 * @brief Segmented Sieve-iZm algorithm for prime generation up to n.
 *
 * @description:
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
    // Bounds check:
    assert(n > 10 && n <= pow(10, 12) && "Input must be in the range (10, 10^12).");

    // if n < 10000, return SiZ(n), doesn't worth segmenting
    if (n < 10000)
        return SiZ(n);

    // * 1. Initialization:
    // Initialize primes array with enough capacity to avoid reallocs
    UI64_ARRAY *primes = ui64_init(est_pi_n(n));
    assert(primes && "Memory allocation failed for primes array in SiZm.");

    // Compute vx wheel size, maximum 6 primes (> 3)
    int vx = iZm_compute_limited_vx(n, 6);
    // Initialize iZm/vx, containing pre-sieved base bitmaps and root primes
    IZM *iZm = iZm_init(vx, sqrt(n) + 1);
    if (!iZm)
    {
        ui64_free(&primes);
        return NULL;
    }

    // Add the pre-sieved k primes to primes array
    int k = 0;
    while ((6 * vx) % iZm->root_primes->array[k] == 0)
        ui64_push(primes, iZm->root_primes->array[k++]);

    // * 2. Process vx segments for y in range [0:y_limit]
    uint64_t x_n = n / 6 + 1; // max x value up to n
    int y_limit = x_n / vx;   // limit for y iterations

    // Initialize active bitmaps for marking composites in segments
    BITMAP *x5 = bitmap_init(vx + 8, 0);
    BITMAP *x7 = bitmap_init(vx + 8, 0);

    uint64_t yvx = 0; // current base value (y * vx)
    for (int y = 0; y <= y_limit; y++)
    {
        // * a. Reset active bitmaps to base state
        memcpy(x5->data, iZm->base_x5->data, x5->byte_size);
        memcpy(x7->data, iZm->base_x7->data, x7->byte_size);

        // local x limit adjusted for last segment
        uint64_t x_limit = (y == y_limit) ? (x_n % vx) : vx;
        // local root limit for current segment
        uint64_t root_limit = sqrt(6 * (yvx + x_limit)) + 1;

        // * b. Mark composites of root primes in current segment
        for (int i = k; i < iZm->root_primes->count; i++)
        {
            uint64_t p = iZm->root_primes->array[i]; // current root prime
            if (p > root_limit)
                break;

            // mark composites of p in current segment
            bitmap_clear_steps_simd(x5, p, iZm_solve_for_xp(-1, p, vx, y), x_limit);
            bitmap_clear_steps_simd(x7, p, iZm_solve_for_xp(1, p, vx, y), x_limit);
        }

        // * c. Collect unmarked indices as primes
        for (uint64_t x = 2; x <= x_limit; x++)
        {
            if (bitmap_get_bit(x5, x)) // i.e. iZ- prime
                ui64_push(primes, iZ(yvx + x, -1));

            if (bitmap_get_bit(x7, x)) // i.e. iZ+ prime
                ui64_push(primes, iZ(yvx + x, 1));
        }

        yvx += vx; // increment yvx for next segment
    }

    // * 3. Clean up and finalize
    iZm_free(&iZm);
    bitmap_free(&x5);
    bitmap_free(&x7);

    // Handle edge case: if last prime > n, remove it
    if (primes->array[primes->count - 1] > n)
        ui64_pop(primes);

    // Trim excess memory in primes array
    ui64_resize_to_fit(primes);

    return primes;
}

// =========================================================
// * SiZ Range Variants
// =========================================================

/**
 * @brief Count or stream primes in an arbitrary numeric range using Sieve-iZm.
 *
 * This function counts primes in the interval [Zs, Ze], where Zs is provided
 * as a decimal string and Ze = Zs + range - 1. It maps the numeric bounds
 * into the iZ index space (6x ± 1), partitions that space into segments of
 * length vx, and for each segment invokes the Sieve-iZm machinery via VX
 * segment objects.
 *
 * When an output file path is provided in @p input_range->filepath, the
 * function additionally streams all primes in the requested interval to that
 * file in ascending order, reconstructing them from per-segment prime gaps.
 * Otherwise, it operates in counting mode only.
 *
 * @param input_range Pointer to an INPUT_SIEVE_RANGE structure describing the
 *        start value (as a decimal string), the range length, the optional
 *        output filepath, and Miller–Rabin configuration.
 * @return The total number of primes found in [Zs, Ze] on success, or 0 on
 *         invalid input, allocation failure, or I/O error.
 */
uint64_t SiZ_stream(INPUT_SIEVE_RANGE *input_range)
{
    assert(input_range && input_range->start && input_range->range >= 100 &&
           "Invalid INPUT_SIEVE_RANGE passed to SiZ_stream.");

    int stream_mode = (input_range->filepath != NULL);
    FILE *output = NULL;
    if (stream_mode)
    {
        output = fopen(input_range->filepath, "w");
        if (output == NULL)
        {
            log_error("Failed to open output file: %s", input_range->filepath);
            return 0;
        }
    }

    uint64_t total = 0; // output: total prime count

    int vx = VX6; // Use VX6 segment size (1,616,615) for optimal results
    // Miller-Rabin rounds, bounded [5, 50]
    int mr_rounds = MIN(MAX(input_range->mr_rounds, 5), 50);

    // Parse numeric bounds into mpz
    mpz_t Zs, Ze, Xs, Xe, Ys, Ye;
    mpz_init(Zs);
    mpz_init(Ze);
    mpz_init(Xs);
    mpz_init(Xe);
    mpz_init(Ys);
    mpz_init(Ye);

    // read start value Zs
    if (mpz_set_str(Zs, input_range->start, 10) != 0)
    {
        log_error("Invalid numeric string for start.");
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return 0;
    }

    // Ze = Zs + range - 1
    mpz_add_ui(Ze, Zs, input_range->range - 1);

    // Convert bounds to iZ index space:
    mpz_fdiv_q_ui(Xs, Zs, 6); // Xs = Zs/6
    // if Zs % 6 > 1, increment Xs
    if (mpz_fdiv_ui(Zs, 6) > 1)
        mpz_add_ui(Xs, Xs, 1);

    mpz_fdiv_q_ui(Xe, Ze, 6); // Xe = Ze/6

    // Ys = Xs / vx, Ye = Xe / vx
    mpz_fdiv_q_ui(Ys, Xs, vx);
    mpz_fdiv_q_ui(Ye, Xe, vx);

    // start_x = (Xs % vx), end_x = (Xe % vx)
    int start_x = mpz_fdiv_ui(Xs, vx);
    int end_x = mpz_fdiv_ui(Xe, vx);

    // if Ys = 0, use sieve_iZm for the first segment
    if (mpz_cmp_ui(Ys, 0) == 0)
    {
        uint64_t limit = mpz_cmp_ui(Ye, 0) > 0 ? vx : end_x;
        UI64_ARRAY *primes = SiZm(limit * 6 + 1);
        total += primes->count;
        uint64_t s = mpz_get_ui(Zs);

        for (int i = 0; i < primes->count; i++)
        {
            // Skip primes < Zs
            if (primes->array[i] < s)
            {
                total--;
                continue;
            }

            // output collected primes in stream mode
            if (stream_mode)
            {
                fprintf(output, "%llu ", primes->array[i]);
            }
        }

        ui64_free(&primes);
        mpz_add_ui(Ys, Ys, 1); // increment Ys for the next segment
    }

    // If Ze < iZ(vx, 1), we are done
    if (mpz_cmp_ui(Ze, iZ(vx, 1)) < 0)
    {
        // cleanup
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return total;
    }

    // Initialize iZm assets
    // if Ze < uint64_t max, set root_limit = sqrt(Ze) + 1, else set to INT32_MAX
    uint64_t root_limit = mpz_cmp_ui(Ze, UINT64_MAX) < 0 ? sqrt(mpz_get_ui(Ze)) + 1 : INT32_MAX;
    IZM *iZm = iZm_init(vx, root_limit);
    if (!iZm)
    {
        // check logs for errors
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return 0;
    }

    // Process remaining segments for y in (Ys:Ye)
    int y_range = mpz_get_ui(Ye) - mpz_get_ui(Ys);
    mpz_t prime_z;
    mpz_init(prime_z);

    for (int i = 0; i <= y_range; i++)
    {
        VX_SEG *vx_obj = vx_init(vx, mpz_get_str(NULL, 10, Ys), mr_rounds);
        if (!vx_obj)
        {
            // check logs for errors
            iZm_free(&iZm);
            mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, prime_z, NULL);
            return 0;
        }

        vx_obj->start_x = i == 0 ? start_x : 1;
        vx_obj->end_x = i == y_range ? end_x : vx;

        // Perform full sieve on the segment and update total count
        vx_full_sieve(iZm, vx_obj, stream_mode);
        total += vx_obj->p_count;

        // output collected primes
        if (stream_mode)
        {
            mpz_add_ui(prime_z, vx_obj->yvx, vx_obj->start_x - 1);
            iZ_mpz(prime_z, prime_z, 1);
            // get size of prime_z in base 2
            size_t prime_size = mpz_sizeinbase(prime_z, 2) + 1;
            char buffer[prime_size];

            for (int j = 0; j < vx_obj->p_count; j++)
            {
                mpz_add_ui(prime_z, prime_z, vx_obj->p_gaps->array[j]);
                gmp_snprintf(buffer, sizeof(buffer), "%Zd ", prime_z);
                fputs(buffer, output);
            }
        }

        vx_free(&vx_obj);
        mpz_add_ui(Ys, Ys, 1); // increment Ys for the next segment
    }

    if (stream_mode)
        fclose(output);

    // Handle edge cases:
    // if Ys > 0 and Zs % 6 <= 1
    if (mpz_cmp_ui(Ys, 0) > 0 && mpz_fdiv_ui(Zs, 6) <= 1)
    {
        // handle edge case: if iZ(Xs, -1) < Zs and prime, decrement total
        iZ_mpz(prime_z, Xs, -1);
        if (mpz_cmp(prime_z, Zs) < 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }
    // if Ye > 0 and Ze % 6 <= 1
    if (mpz_cmp_ui(Ye, 0) > 0 && mpz_fdiv_ui(Ze, 6) <= 1)
    {
        // handle edge case: if iZ(Xe, 1) > Ze and prime, decrement total
        iZ_mpz(prime_z, Xe, 1);
        if (mpz_cmp(prime_z, Ze) > 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }

    // Cleanup
    iZm_free(&iZm);
    mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, prime_z, NULL);

    return total;
}

/**
 * @brief Multi-process prime counting over a numeric range using Sieve-iZm.
 *
 * This function parallelizes prime counting over the interval [Zs, Ze]
 * (interpreted from @p input_range) by partitioning the corresponding iZ
 * index space into VX segments and distributing contiguous blocks of segments
 * across multiple processes. Each child process owns its own cloned IZM
 * instance and VX segments, performs a full Sieve-iZm pass over its assigned
 * segments, and returns a local prime count to the parent via a pipe.
 *
 * The parent process aggregates all child counts, applies boundary
 * corrections for endpoints that do not align exactly with 6x ± 1, and
 * returns the total number of primes in [Zs, Ze]. Any failure to create
 * pipes, fork children, or initialize per-process resources causes the
 * function to log an error and return 0.
 *
 * @param input_range Pointer to an INPUT_SIEVE_RANGE structure describing the
 *        numeric interval and Miller–Rabin configuration.
 * @param cores_num Requested number of worker processes. The actual number
 *        used is clamped to the available CPU cores and the number of VX
 *        segments in the range.
 * @return The total number of primes found in [Zs, Ze] on success, or 0 on
 *         any error or child-process failure.
 */
uint64_t SiZ_count(INPUT_SIEVE_RANGE *input_range, int cores_num)
{
    assert(input_range && input_range->start && input_range->range > 100 &&
           "Invalid INPUT_SIEVE_RANGE passed to SiZ_count.");

    uint64_t total = 0;
    int vx = VX6;             // use VX6 segment size for optimal results
    int vx_interval = 6 * vx; // covers a range of 9,699,690
    cores_num = MIN(cores_num, get_cpu_cores_count());
    input_range->filepath = NULL; // disable streaming for counting mode

    uint64_t range = input_range->range;
    int total_segments = (range / vx_interval) + 1;

    // if total_segments < cores_num, adjust cores_num
    if (total_segments < cores_num)
    {
        cores_num = total_segments;
    }

    if (cores_num == 1)
    {
        return SiZ_stream(input_range);
    }

    // Parse numeric bounds into mpz
    mpz_t Zs, Ze, Xs, Xe, Ys, Ye;
    mpz_init(Zs);
    mpz_init(Ze);
    mpz_init(Xs);
    mpz_init(Xe);
    mpz_init(Ys);
    mpz_init(Ye);

    if (mpz_set_str(Zs, input_range->start, 10) != 0)
    {
        log_error("Invalid numeric string for start.");
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return 0;
    }

    // Convert bounds to iZ index space:
    // Ze = Zs + range - 1
    mpz_add_ui(Ze, Zs, input_range->range - 1);
    mpz_fdiv_q_ui(Xs, Zs, 6); // Xs = Zs/6
    // if Zs % 6 > 1, increment Xs
    if (mpz_fdiv_ui(Zs, 6) > 1)
        mpz_add_ui(Xs, Xs, 1);

    mpz_fdiv_q_ui(Xe, Ze, 6); // Xe = Ze/6

    // Ys = Xs / vx, Ye = Xe / vx
    mpz_fdiv_q_ui(Ys, Xs, vx);
    mpz_fdiv_q_ui(Ye, Xe, vx);

    // start_x = (Xs % vx), end_x = (Xe % vx)
    int start_x = mpz_fdiv_ui(Xs, vx);
    int end_x = mpz_fdiv_ui(Xe, vx);

    // if Ys = 0, use sieve_iZm for the first segment
    if (mpz_cmp_ui(Ys, 0) == 0)
    {
        uint64_t limit = mpz_cmp_ui(Ye, 0) > 0 ? vx : end_x;
        UI64_ARRAY *primes = SiZm(limit * 6 + 1);
        total += primes->count;
        uint64_t s = mpz_get_ui(Zs);

        for (int i = 0; i < primes->count; i++)
        {
            // Skip primes < Zs
            if (primes->array[i] < s)
            {
                total--;
                continue;
            }
        }

        ui64_free(&primes);
        mpz_add_ui(Ys, Ys, 1); // increment Ys for the next segment
        total_segments--;
    }

    // Multi-process processing of remaining segments
    int segments_per_core = total_segments / cores_num;
    int remainder_segments = total_segments % cores_num;

    uint64_t root_limit = mpz_cmp_ui(Ze, INT64_MAX) < 0 ? sqrt(mpz_get_ui(Ze)) + 1 : INT32_MAX;
    IZM *iZm = iZm_init(vx, root_limit);
    if (!iZm)
    {
        // check logs for errors
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return 0;
    }

    int pipe_fds[cores_num][2];
    pid_t pids[cores_num];
    for (int i = 0; i < cores_num; i++)
    {
        pids[i] = -1;
        pipe_fds[i][0] = -1;
        pipe_fds[i][1] = -1;
    }

    for (int core = 0; core < cores_num; core++)
    {
        if (pipe(pipe_fds[core]) == -1)
        {
            log_error("SiZ_count: Failed to create pipe for core %d. Aborting.", core);
            // cleanup and abort on any pipe error
            for (int j = 0; j <= core; j++)
            {
                if (pipe_fds[j][0] != -1)
                    close(pipe_fds[j][0]);
                if (pipe_fds[j][1] != -1)
                    close(pipe_fds[j][1]);
            }
            mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
            iZm_free(&iZm);
            return 0;
        }

        pids[core] = fork();
        if (pids[core] < 0)
        {
            log_error("SiZ_count: Failed to fork process for core %d. Aborting.", core);
            // close this core's pipe and any previous ones, then exit
            for (int j = 0; j <= core; j++)
            {
                if (pipe_fds[j][0] != -1)
                    close(pipe_fds[j][0]);
                if (pipe_fds[j][1] != -1)
                    close(pipe_fds[j][1]);
            }
            mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
            iZm_free(&iZm);
            return 0;
        }

        if (pids[core] == 0)
        {
            // Child process
            // Close unrelated pipes
            for (int j = 0; j < cores_num; j++)
            {
                if (j != core && pipe_fds[j][0] != -1)
                    close(pipe_fds[j][0]);
                if (j != core && pipe_fds[j][1] != -1)
                    close(pipe_fds[j][1]);
            }
            close(pipe_fds[core][0]); // close read end

            // Compute this child's starting Y and number of segments
            int offset = core * segments_per_core + (core < remainder_segments ? core : remainder_segments);
            int local_segments = segments_per_core + (core < remainder_segments ? 1 : 0);

            uint64_t child_total = 0;

            if (local_segments > 0)
            {
                mpz_t local_Ys;
                mpz_init(local_Ys);
                mpz_set(local_Ys, Ys);
                mpz_add_ui(local_Ys, local_Ys, offset);

                // Each child has its own IZM to avoid data races
                IZM *iZm_local = iZm_clone(iZm);

                for (int i = 0; i < local_segments; i++)
                {
                    VX_SEG *vx_obj = vx_init(vx, mpz_get_str(NULL, 10, local_Ys), input_range->mr_rounds);

                    // Apply boundary handling only for first/last overall segments
                    vx_obj->start_x = (core == 0 && i == 0) ? start_x : 1;
                    vx_obj->end_x = (core == cores_num - 1 && i == local_segments - 1) ? end_x : vx;

                    vx_full_sieve(iZm_local, vx_obj, 0);
                    child_total += vx_obj->p_count;

                    vx_free(&vx_obj);
                    mpz_add_ui(local_Ys, local_Ys, 1);
                }

                iZm_free(&iZm_local);
                mpz_clear(local_Ys);
            }

            // Send result to parent
            ssize_t written = write(pipe_fds[core][1], &child_total, sizeof(child_total));
            if (written != sizeof(child_total))
            {
                log_error("SiZ_count: Child %d failed to write result.", core);
            }

            close(pipe_fds[core][1]);
            exit(0);
        }
        else
        {
            // Parent process
            close(pipe_fds[core][1]); // close write end
            pipe_fds[core][1] = -1;
        }
    }
    iZm_free(&iZm);

    // Collect results
    for (int core = 0; core < cores_num; core++)
    {
        if (pids[core] > 0 && pipe_fds[core][0] != -1)
        {
            uint64_t child_total = 0;
            ssize_t bytes_read = read(pipe_fds[core][0], &child_total, sizeof(child_total));
            if (bytes_read == sizeof(child_total))
            {
                total += child_total;
            }
            else if (bytes_read == 0)
            {
                log_error("SiZ_count: Child %d closed pipe without sending result.", core);
            }
            else if (bytes_read < 0)
            {
                log_error("SiZ_count: Failed to read result from child %d.", core);
            }
            else
            {
                log_error("SiZ_count: Partial read from child %d (got %zd bytes).", core, bytes_read);
            }

            close(pipe_fds[core][0]);
            pipe_fds[core][0] = -1;

            int status;
            waitpid(pids[core], &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {
                log_error("SiZ_count: Child %d exited with status %d.", core, WEXITSTATUS(status));
                // propagate child failure as an overall error
                mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
                return 0;
            }
            else if (WIFSIGNALED(status))
            {
                log_error("SiZ_count: Child %d terminated by signal %d.", core, WTERMSIG(status));
                mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
                return 0;
            }
        }
        else if (pipe_fds[core][0] != -1)
        {
            close(pipe_fds[core][0]);
            pipe_fds[core][0] = -1;
        }
    }

    // Handle edge cases:
    mpz_t prime_z;
    mpz_init(prime_z);
    // if Ys > 0 and Zs % 6 <= 1
    if (mpz_cmp_ui(Ys, 0) > 0 && mpz_fdiv_ui(Zs, 6) <= 1)
    {
        // if iZ(Xs, -1) < Zs and prime, decrement total
        iZ_mpz(prime_z, Xs, -1);
        if (mpz_cmp(prime_z, Zs) < 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }
    // if Ye > 0 and Ze % 6 <= 1
    if (mpz_cmp_ui(Ye, 0) > 0 && mpz_fdiv_ui(Ze, 6) <= 1)
    {
        // if iZ(Xe, 1) > Ze and prime, decrement total
        iZ_mpz(prime_z, Xe, 1);
        if (mpz_cmp(prime_z, Ze) > 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }
    mpz_clear(prime_z);

    // Cleanup
    mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
    return total;
}
