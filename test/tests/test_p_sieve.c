#include <test_api.h>

// Sieve function type, takes uint64_t limit and returns a UI64_ARRAY pointer

// * List of available algorithms
const SIEVE_MODEL _SoE = {SoE, "SoE"};    // * Sieve of Eratosthenes
const SIEVE_MODEL _SSoE = {SSoE, "SSoE"}; // * Segmented Sieve of Eratosthenes
const SIEVE_MODEL _SoEu = {SoEu, "SoEu"}; // * Sieve of Euler
const SIEVE_MODEL _SoS = {SoS, "SoS"};    // * Sieve of Sundaram
const SIEVE_MODEL _SoA = {SoA, "SoA"};    // * Sieve of Atkin
const SIEVE_MODEL _SiZ = {SiZ, "SiZ"};    // * Sieve-iZ
const SIEVE_MODEL _SiZm = {SiZm, "SiZm"}; // * Sieve-iZm

// A variant of SiZm, sieving the vy dimension for improved performance, however,
// it returns unordered list of primes, so it will fail the integrity test
UI64_ARRAY *SiZm_vy(uint64_t n);
const SIEVE_MODEL _SiZm_vy = {SiZm_vy, "SiZm_vy"}; // * Sieve-iZm_vy

SIEVE_MODEL SIEVE_MODELS[] = {
    _SoE,
    _SSoE,
    _SoEu,
    _SoS,
    _SoA,
    _SiZ,
    _SiZm,
    // _SiZm_vy,
};

int models_count = sizeof(SIEVE_MODELS) / sizeof(SIEVE_MODELS[0]);

// =======================================================================
// * Testing Sieve Models Integrity
// =======================================================================
/**
 * @brief Tests the integrity of the algorithms in the SIEVE_MODELS array.
 *
 * This function iterates through the list of sieve models in SIEVE_MODELS,
 * generates the prime numbers up to `n` using each model,
 * then compares the SHA-256 hash of the resulting prime array.
 * If all models produce the same hash, the integrity test passes.
 *
 * @param SIEVE_MODELS A structure containing the list of sieve models to be tested.
 * @param n The upper limit for the prime number generation.
 * @return 1 if all hashes match, 0 if a hash mismatch is detected.
 */
static int test_sieve_integrity(uint64_t n, int verbose)
{
    // Store hashes for each model
    unsigned char result_hashes[models_count][SHA256_DIGEST_LENGTH];

    if (verbose)
    {
        print_line(100, '-');
        printf("| %-12s | %-12s | %-12s | %s\n", "Sieve Model", "Limit (N)", "Primes Count", "SHA-256");
        print_line(100, '-');
    }

    for (int i = 0; i < models_count; i++)
    {
        SIEVE_MODEL sieve_model = SIEVE_MODELS[i];

        // Call the sieve function
        UI64_ARRAY *primes = sieve_model.function(n);
        if (primes == NULL)
        {
            printf("Failed to generate primes with %s\n", sieve_model.name);
            return 0;
        }
        // Compute the SHA-256 hash of the primes
        ui64_compute_hash(primes);

        // Print the result row
        if (verbose)
        {
            printf("| %-12s | %-12llu | %-12d | ", sieve_model.name, n, primes->count);
            print_sha256_hash(primes->sha256);
        }

        // Store the hash in the results array
        memcpy(result_hashes[i], primes->sha256, 32);

        ui64_free(&primes); // Free the UI64_ARRAY after use
    }

    // Compare all hashes to the first hash (SoE)
    int mismatch = 0;
    for (int i = 1; i < models_count; i++)
    {
        if (memcmp(result_hashes[0], result_hashes[i], 32) != 0)
        {
            mismatch++;
        }
    }

    return (mismatch == 0) ? 1 : 0;
}

/**
 * @brief Tests the integrity of all sieve models in SIEVE_MODELS.
 *
 * This function runs integrity tests for all sieve algorithms defined in the
 * SIEVE_MODELS array over a set of predefined limits (10^3, 10^6, 10^9).
 * It verifies that all algorithms produce identical SHA-256 hashes for the
 * generated prime number arrays, ensuring consistency across implementations.
 *
 * @param verbose If non-zero, prints detailed test results to stdout.
 * @return 1 if all tests pass (hashes match), 0 if any test fails (hash mismatch).
 */
int TEST_SIEVE_MODELS_INTEGRITY(int verbose)
{
    print_line(60, '*');
    print_centered_text(" SIEVE ALGORITHMS INTEGRITY TEST ", 60, '=');
    print_line(60, '*');
    fflush(stdout);

    int result = 1;
    // Test the integrity of each sieve model for limits 10^3, 10^6, 10^9
    for (int e = 3; e < 10; e += 3)
        result = test_sieve_integrity(pow(10, e), verbose);

    print_line(60, '*');
    if (result)
    {
        printf("[SUCCESS] All hashes match! Implementations seem OK\n");
    }
    else
    {
        printf("[FAILURE] Hash mismatch detected. Check failed models :\\\n");
    }
    print_line(60, '*');

    return result;
}

// =======================================================================
// * Benchmarking Sieve Models
// =======================================================================
/**
 * @brief Measures the execution time of a given sieve algorithm.
 *
 * This function takes a sieve algorithm and an upper limit `n`, runs the algorithm,
 * and measures the time taken to execute it. It prints the algorithm name, the value of `n`,
 * the count of prime numbers found, the last prime number in the list, and the time taken in seconds.
 * It returns the execution time in microseconds.
 *
 * @param algorithm The sieve algorithm to be measured.
 * @param n The upper limit for the sieve algorithm.
 * @return The execution time in microseconds.
 */
static size_t measure_sieve_time(SIEVE_MODEL model, SIEVE_LIMIT limit)
{
    uint64_t n = pow(limit.base, limit.exp);
    clock_t start, end;
    double cpu_time_used;
    UI64_ARRAY *primes;

    start = clock();            // Start time
    primes = model.function(n); // Run time
    end = clock();              // End time

    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    char n_str[32];
    snprintf(n_str, sizeof(n_str), "%d^%d", limit.base, limit.exp);
    printf("| %-16s", n_str);
    printf("| %-16d", primes->count);
    printf("| %-16lld", primes->array[primes->count - 1]);
    printf("| %-16f\n", cpu_time_used); // time in seconds
    fflush(stdout);

    ui64_free(&primes);
    return (size_t)(cpu_time_used * 1000000); // time in microseconds;
}

/**
 * @brief Saves the results of the sieve models to a file.
 *
 * This function writes the results to a file named by timestamp in the default output directory.
 * The results include metadata about the test range and the results of each sieve model.
 *
 * @param all_results A 2D array containing the results of each sieve model.
 * @param limits_array An array of SIEVE_LIMIT structures defining the test limits.
 * @param tests_count The number of tests conducted.
 */
static void save_results_file(int all_results[][32], SIEVE_LIMIT limits_array[], int tests_count)
{
    // this function writes the results to a file named by timestamp in the default output directory
    // the output is in the format:
    // Test Limits: [limit1, limit2, ...]
    // Test Results:
    // Sieve Model1: [time1, time2, ...]
    // Sieve Model2: [time1, time2, ...]

    struct stat st = {0};
    if (stat(DIR_output, &st) == -1)
        mkdir(DIR_output, 0700);

    // Get the current timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp) - 1, "%d%H%M%S", t);

    // Create the output file path
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/psieve_%s.txt", DIR_output, timestamp);

    // Open the file for writing
    FILE *fp = fopen(file_path, "w");
    if (fp == NULL)
    {
        log_error("Failed to open file");
        return;
    }

    // Write the test limits array
    fprintf(fp, "Test Limits: [%d^%d", limits_array[0].base, limits_array[0].exp);

    for (int i = 1; i < tests_count; i++)
    {
        SIEVE_LIMIT limit = limits_array[i];
        fprintf(fp, ", %d^%d", limit.base, limit.exp);
    }
    fprintf(fp, "]\n");

    // Write results header
    fprintf(fp, "Test Results:\n");
    // Write the results array for each model
    for (int i = 0; i < models_count; i++)
    {
        SIEVE_MODEL sieve_model = SIEVE_MODELS[i];
        fprintf(fp, "%s: [", sieve_model.name);
        for (int j = 0; j < tests_count; j++)
        {
            fprintf(fp, "%d", all_results[i][j]);
            if (j < tests_count - 1)
                fprintf(fp, ", ");
        }
        fprintf(fp, "]\n");
    }

    // Close the file
    fclose(fp);
    printf("\nResults saved to %s\n", file_path);
}

/**
 * @brief Benchmarks the performance of different sieve algorithms over a range of exponents.
 *
 * This function measures the execution time of various sieve algorithms for a range of values
 * determined by the base raised to the power of exponents from min_exp to max_exp. The results
 * are printed and optionally saved to a file.
 *
 * @param sieve_models A structure containing the list of sieve algorithms to benchmark.
 * @param base The base value to be raised to the power of exponents.
 * @param min_exp The minimum exponent value.
 * @param max_exp The maximum exponent value.
 * @param save_results A flag indicating whether to save the results to a file named by timestamp in the output directory.
 */
void BENCHMARK_SIEVE_MODELS(int save_results)
{
    int times_array[models_count][32];
    SIEVE_LIMIT limits_array[32];
    int tests_count = 0;
    // define limits: 10^4 to 10^10
    for (int exp = 4; exp < 10; exp++)
    {
        limits_array[tests_count++] = (SIEVE_LIMIT){10, exp};
    }

    for (int i = 0; i < models_count; i++)
    {
        SIEVE_MODEL model = SIEVE_MODELS[i];

        // results array: [time in seconds]
        int results[32];
        int k = 0;

        printf("\nAlgorithm: %s\n", model.name);
        print_line(75, '-');
        printf("| %-16s", "N (Limit)");
        printf("| %-16s", "Primes Count");
        printf("| %-16s", "Last Prime");
        printf("| %-16s\n", "Time (s)");
        print_line(75, '-');

        // warmup
        UI64_ARRAY *primes = model.function(10000);
        ui64_free(&primes);

        for (int j = 0; j < tests_count; j++)
            results[k++] = measure_sieve_time(model, limits_array[j]); // returns time in microseconds

        print_line(75, '-');
        fflush(stdout);

        for (int j = 0; j < k; j++)
            times_array[i][j] = results[j];
    }

    // save results file
    if (save_results)
        save_results_file(times_array, limits_array, tests_count);
}

// =======================================================================
// * Testing SiZ_stream
// =======================================================================
int TEST_SiZ_stream(int verbose)
{
    int failed_tests = 0;

    uint64_t test_count = 0;
    struct timeval start, end;
    double elapsed_seconds = 0;

    mpz_t end_num;
    mpz_init(end_num);

    print_line(60, '*');
    print_centered_text("TESTING SiZ_stream", 60, '=');
    print_line(60, '*');
    fflush(stdout);

    char start_str[128] = "0";
    uint64_t test_range = 1000000;
    uint64_t expected_count = 78498;

    INPUT_SIEVE_RANGE input_range = {
        .start = start_str,
        .range = test_range,
        .mr_rounds = MR_ROUNDS,
        .filepath = "./output/SiZ_stream_test1.txt",
    };

    mpz_set_str(end_num, input_range.start, 10);
    mpz_add_ui(end_num, end_num, test_range);

    gettimeofday(&start, NULL);
    test_count = SiZ_stream(&input_range);
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Check result
    if (test_count != expected_count)
        failed_tests++;

    printf("Test 1: Streaming primes in range [%s:%s]\n", input_range.start, mpz_get_str(NULL, 10, end_num));
    if (verbose)
    {
        printf("%-32s: %llu\n", "Expected primes count", expected_count);
        printf("%-32s: %llu\n", "Result primes count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        printf("%-32s: %s\n", "Output File", input_range.filepath);
    }
    else
    {
        if (test_count != expected_count)
        {
            printf("Expected primes count: %llu, Got: %llu\n", expected_count, test_count);
        }
    }
    // ===================================
    // Test 2
    print_line(60, '=');
    input_range.start = "1000000000000";
    expected_count = 36249;
    input_range.filepath = "./output/SiZ_stream_test2.txt";

    mpz_set_str(end_num, input_range.start, 10);
    mpz_add_ui(end_num, end_num, test_range);

    gettimeofday(&start, NULL);
    test_count = SiZ_stream(&input_range);
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Check result
    if (test_count != expected_count)
        failed_tests++;

    printf("Test 2: Streaming primes in range [%s:%s]\n", input_range.start, mpz_get_str(NULL, 10, end_num));
    if (verbose)
    {
        printf("%-32s: %llu\n", "Expected primes count", expected_count);
        printf("%-32s: %llu\n", "Result primes count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        printf("%-32s: %s\n", "Output File", input_range.filepath);
    }
    else
    {
        if (test_count != expected_count)
        {
            printf("Expected primes count: %llu, Got: %llu\n", expected_count, test_count);
        }
    }

    print_line(60, '*');
    int result = (failed_tests == 0) ? 1 : 0;
    if (result)
    {
        printf("[SUCCESS] SiZ_stream tests passed!\n");
    }
    else
    {
        printf("[FAILURE] SiZ_stream tests failed :\\\n");
    }
    print_line(60, '*');
    fflush(stdout);

    mpz_clear(end_num);
    return result;
}

// =======================================================================
// * Testing SiZm_count
// =======================================================================

// testing SiZm_count for 10^9 range both single and multi-core
// additional test for 10^12 range can be added but it takes longer time
int TEST_SiZ_count(int verbose)
{
    int result = 1;

    int cores_num = MAX_CORES;
    uint64_t test_count = 0;
    struct timeval start, end;
    double elapsed_seconds = 0;

    mpz_t end_num;
    mpz_init(end_num);

    print_line(60, '*');
    printf("TESTING SiZm_count\n");
    print_line(60, '*');

    // * Test 1: 0 to 10^9 single-core
    uint64_t interval = pow(10, 9);

    INPUT_SIEVE_RANGE input_range = {
        .start = "0",
        .range = interval,
        .mr_rounds = MR_ROUNDS,
        .filepath = NULL,
    };

    // set end_num = start_num + interval
    mpz_set_str(end_num, input_range.start, 10);
    mpz_add_ui(end_num, end_num, interval);

    printf("Test 1: Counting primes in range [%s:%s] using single core\n", input_range.start, mpz_get_str(NULL, 10, end_num));
    fflush(stdout);

    gettimeofday(&start, NULL);
    test_count = SiZ_count(&input_range, 1);
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    uint64_t expected_count = 50847534;
    result = test_count == expected_count;

    if (verbose)
    {
        printf("%-32s: %llu\n", "Expected prime count", expected_count);
        printf("%-32s: %llu\n", "Result prime count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        fflush(stdout);
    }

    // ===========
    // * Test 2: 0 to 10^9 multi-core
    print_line(30, '=');
    printf("Test 2: Counting primes in range [%s:%s] using %d cores\n", input_range.start, mpz_get_str(NULL, 10, end_num), cores_num);
    fflush(stdout);

    gettimeofday(&start, NULL);
    test_count = SiZ_count(&input_range, cores_num);
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    mpz_set_str(end_num, input_range.start, 10);
    mpz_add_ui(end_num, end_num, interval);

    expected_count = 50847534;
    result = test_count == expected_count;

    if (verbose)
    {
        printf("%-32s: %llu\n", "Expected prime count", expected_count);
        printf("%-32s: %llu\n", "Result prime count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        fflush(stdout);
    }
    // =========

    // * Test 3: 0 to 10^12 range multi-core (optional, uncomment to run)
    // print_line(30, '=');
    // printf("Test 3: Counting primes in range [%s:%s] using %d cores\n", input_range.start, mpz_get_str(NULL, 10, end_num), cores_num);

    // interval = pow(10, 12);
    // input_range.range = interval;
    // mpz_set_str(end_num, input_range.start, 10);
    // mpz_add_ui(end_num, end_num, input_range.range);

    // gettimeofday(&start, NULL);
    // test_count = SiZ_count(&input_range, cores_num);
    // gettimeofday(&end, NULL);
    // elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // mpz_set_str(end_num, input_range.start, 10);
    // mpz_add_ui(end_num, end_num, interval);

    // expected_count = 37607912018;
    // result = test_count == expected_count;

    // if (verbose)
    // {
    //     printf("%-32s: %llu\n", "Expected prime count", expected_count);
    //     printf("%-32s: %llu\n", "Result prime count", test_count);
    //     printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
    //     fflush(stdout);
    // }
    // =========

    print_line(60, '*');
    if (result)
    {
        printf("[SUCCESS] SiZm_count tests passed! \n");
    }
    else
    {
        printf("[FAILURE] SiZm_count tests failed :\\\n");
    }
    print_line(60, '*');

    mpz_clear(end_num);
    return result;
}
