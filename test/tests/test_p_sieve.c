#include <tests.h>

// Sieve function type, takes uint64_t limit and returns a UI64_ARRAY pointer
typedef UI64_ARRAY *(*SIEVE_FN)(uint64_t);

// Structure to associate the sieve function with its name
typedef struct
{
    SIEVE_FN function;
    const char name[32];
} SIEVE_MODEL;

// List of available algorithms
const SIEVE_MODEL _SoE = {SoE, "SoE"};             // * Sieve of Eratosthenes
const SIEVE_MODEL _SSoE = {SSoE, "SSoE"};          // * Segmented Sieve of Eratosthenes
const SIEVE_MODEL _SoEu = {SoEu, "SoEu"};          // * Sieve of Euler
const SIEVE_MODEL _SoS = {SoS, "SoS"};             // * Sieve of Sundaram
const SIEVE_MODEL _SoA = {SoA, "SoA"};             // * Sieve of Atkin
const SIEVE_MODEL _SiZ = {SiZ, "SiZ"};             // * Sieve-iZ
const SIEVE_MODEL _SiZm = {SiZm, "SiZm"};          // * Sieve-iZm
const SIEVE_MODEL _SiZ_210 = {SiZ_210, "SiZ_210"}; // * Sieve-iZ_210

SIEVE_MODEL SIEVE_MODELS[] = {
    // _SoE,
    // _SSoE,
    // _SoEu,
    // _SoS,
    // _SoA,
    _SiZ,
    _SiZm,
    // _SiZ_210,
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
static size_t print_sieve_time(SIEVE_MODEL model, int base, int exp)
{
    uint64_t n = pow(base, exp);
    clock_t start, end;
    double cpu_time_used;
    UI64_ARRAY *primes;

    start = clock();            // Start time
    primes = model.function(n); // Run time
    end = clock();              // End time

    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    char n_str[32];
    snprintf(n_str, sizeof(n_str), "%d^%d", base, exp);
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
 * This function creates a directory if it does not exist, generates a timestamped
 * filename, and writes the results of the sieve models to this file. The results
 * include metadata about the test range and the results of each sieve model.
 *
 * @param sieve_models The sieve models containing the algorithms and their names.
 * @param all_results A 2D array containing the results of the sieve models.
 * @param base The base number used in the test range.
 * @param min_exp The minimum exponent used in the test range.
 * @param max_exp The maximum exponent used in the test range.
 */
static void save_results_file(int all_results[][32], int base, int min_exp, int max_exp)
{

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

    // Write the test range metadata
    fprintf(fp, "Test Range: %d^%d:%d^%d\n", base, min_exp, base, max_exp);

    // Write the results to the file
    for (int i = 0; i < models_count; i++)
    {
        SIEVE_MODEL sieve_model = SIEVE_MODELS[i];

        // P_SIEVE model = sieve_models.models_list[i];

        fprintf(fp, "%s: [", sieve_model.name);
        for (int j = 0; j <= max_exp - min_exp; j++)
        {
            fprintf(fp, "%d", all_results[i][j]);
            if (j < max_exp - min_exp)
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
    int all_results[models_count][32];
    int base = 10, min_exp = 4, max_exp = 9;

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
        UI64_ARRAY *primes = model.function(pow(base, min_exp));
        ui64_free(&primes);

        for (int j = min_exp; j <= max_exp; j++)
        {
            results[k++] = print_sieve_time(model, base, j); // returns time in microseconds
        }
        // additional tests:
        // results[k++] = print_sieve_time(model, 2, 32); // for 2^32
        // results[k++] = print_sieve_time(model, 10, 10); // for 10^10
        print_line(75, '-');
        fflush(stdout);

        for (int j = 0; j < k; j++)
            all_results[i][j] = results[j];
    }

    // save results file
    if (save_results)
        save_results_file(all_results, base, min_exp, max_exp);
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

    mpz_t end_range;
    mpz_init(end_range);

    print_line(60, '*');
    print_centered_text("TESTING SiZ_stream", 60, '=');
    print_line(60, '*');
    fflush(stdout);

    char start_str[128] = "0";
    uint64_t test_range = 1000000000;
    uint64_t expected_count = 50847534;
    mpz_set_str(end_range, start_str, 10);
    mpz_add_ui(end_range, end_range, test_range);

    INPUT_SIEVE_RANGE input_range = {
        .start = start_str,
        .range = test_range,
        .mr_rounds = MR_ROUNDS,
        .filepath = NULL,
    };

    gettimeofday(&start, NULL);
    test_count = SiZ_stream(&input_range);
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Check result
    if (test_count != expected_count)
        failed_tests++;

    if (verbose)
    {
        print_line(30, '=');
        printf("Test 1: Counting primes in range [%s:%s]\n", start_str, mpz_get_str(NULL, 10, end_range));
        printf("Expected prime count: %-16llu\n", expected_count);
        printf("Result prime count:   %-16llu\n", test_count);
        printf("Execution time (s):   %-16f\n", elapsed_seconds);
        fflush(stdout);
    }

    // ===========
    strcpy(start_str, "1000000000");
    expected_count = 47374753;

    mpz_set_str(end_range, start_str, 10);
    mpz_add_ui(end_range, end_range, test_range);

    input_range.start = start_str;

    gettimeofday(&start, NULL);
    test_count = SiZ_stream(&input_range);
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Check result
    if (test_count != expected_count)
        failed_tests++;

    if (verbose)
    {
        print_line(30, '=');
        printf("Test 2: Counting primes in range [%s:%s]\n", start_str, mpz_get_str(NULL, 10, end_range));
        printf("Expected prime count: %-16llu\n", expected_count);
        printf("Result prime count:   %-16llu\n", test_count);
        printf("Execution time (s):   %-16f\n", elapsed_seconds);
        fflush(stdout);
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

    mpz_clear(end_range);
    return result;
}

// =======================================================================
// * Testing SiZm_count
// =======================================================================

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
    printf("TESTING SiZm_count with %d cores\n", cores_num);
    print_line(60, '*');
    fflush(stdout);

    char start_str[] = "0";
    uint64_t interval = pow(10, 9);
    uint64_t expected_count = 50847534;
    // set end_num = start_num + interval
    mpz_set_str(end_num, start_str, 10);
    mpz_add_ui(end_num, end_num, interval);

    INPUT_SIEVE_RANGE input_range = {
        .start = start_str,
        .range = interval,
        .mr_rounds = MR_ROUNDS,
        .filepath = NULL,
    };

    gettimeofday(&start, NULL);
    test_count = SiZ_count(&input_range, cores_num);
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    result = test_count == expected_count;

    if (verbose)
    {
        print_line(30, '=');
        printf("Test 1: Counting primes in range [%s:%s]\n", start_str, mpz_get_str(NULL, 10, end_num));
        printf("Expected prime count: %-16llu\n", expected_count);
        printf("Result prime count:   %-16llu\n", test_count);
        printf("Execution time (s):   %-16f\n", elapsed_seconds);
        fflush(stdout);
    }

    // ===========
    // interval = pow(10, 12);
    // expected_count = 37607912018;

    // mpz_set_str(end_num, start_str, 10);
    // mpz_add_ui(end_num, end_num, interval);

    // input_range.start = start_str;
    // input_range.range = interval;

    // gettimeofday(&start, NULL);
    // test_count = SiZ_count(&input_range, cores_num);
    // gettimeofday(&end, NULL);
    // elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    // result = test_count == expected_count;

    // if (verbose)
    // {
    //     print_line(30, '=');
    //     printf("Test 2: Counting primes in range [%s:%s]\n", start_str, mpz_get_str(NULL, 10, end_num));
    //     printf("Expected prime count: %-16llu\n", expected_count);
    //     printf("Result prime count:   %-16llu\n", test_count);
    //     printf("Execution time (s):   %-16f\n", elapsed_seconds);
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
