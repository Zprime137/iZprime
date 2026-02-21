#include <test_api.h>
#include <inttypes.h>

// Sieve function type, takes uint64_t limit and returns a UI64_ARRAY pointer

// * List of available algorithms
const SIEVE_MODEL _SoE = {SoE, "SoE"};             // * Sieve of Eratosthenes
const SIEVE_MODEL _SSoE = {SSoE, "SSoE"};          // * Segmented Sieve of Eratosthenes
const SIEVE_MODEL _SoEu = {SoEu, "SoEu"};          // * Sieve of Euler
const SIEVE_MODEL _SoS = {SoS, "SoS"};             // * Sieve of Sundaram
const SIEVE_MODEL _SoA = {SoA, "SoA"};             // * Sieve of Atkin
const SIEVE_MODEL _SiZ = {SiZ, "SiZ"};             // * Sieve-iZ
const SIEVE_MODEL _SiZm = {SiZm, "SiZm"};          // * Sieve-iZm
const SIEVE_MODEL _SiZm_vy = {SiZm_vy, "SiZm_vy"}; // * Sieve-iZm_vy (unordered)

// Array of sieve models to be tested and benchmarked,
// uncomment the ones you want to include in the tests and benchmarks
SIEVE_MODEL SIEVE_MODELS[] = {
    _SoE,
    _SSoE,
    _SoEu,
    _SoS,
    _SoA,
    _SiZ,
    _SiZm,
    _SiZm_vy,
};

int models_count = sizeof(SIEVE_MODELS) / sizeof(SIEVE_MODELS[0]);

// =======================================================================
// * Testing Sieve Models Integrity
// =======================================================================
/**
 * @brief Tests the correctness of the algorithms in the SIEVE_MODELS array.
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
    unsigned char baseline_hash[SHA256_DIGEST_LENGTH];
    int baseline_set = 0;
    int tested_models = 0;
    int mismatch = 0;

    if (verbose)
    {
        print_line(100, '-');
        printf("| %-12s | %-12s | %s\n", "Sieve Model", "Primes Count", "SHA-256");
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
        if (primes->ordered == 0) // if the primes array is marked as unordered,
            ui64_sort(primes);    // sort it to get a consistent hash for integrity testing

        // Compute the SHA-256 hash of the primes
        ui64_compute_hash(primes);

        // Print the result row
        if (verbose)
        {
            printf("| %-12s | %-12d | ", sieve_model.name, primes->count);
            print_sha256_hash(primes->sha256);
        }

        // Use the first tested model as baseline, compare all subsequent hashes to it.
        if (!baseline_set)
        {
            memcpy(baseline_hash, primes->sha256, SHA256_DIGEST_LENGTH);
            baseline_set = 1;
        }
        else if (memcmp(baseline_hash, primes->sha256, SHA256_DIGEST_LENGTH) != 0)
        {
            mismatch++;
        }

        tested_models++;

        ui64_free(&primes); // Free the UI64_ARRAY after use
    }

    return (tested_models > 0 && mismatch == 0);
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
    print_test_fn_header("SIEVE MODELS INTEGRITY");
    fflush(stdout);

    int result = 1;
    // Test the integrity of each sieve model for limits 10^3, 10^6, 10^9
    for (int e = 3; e < 10; e += 3)
    {
        printf("\nTesting sieve models integrity for limit 10^%d\n", e);
        result = result && test_sieve_integrity(pow(10, e), verbose);
    }

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
    double elapsed_seconds;
    UI64_ARRAY *primes;

    STOPWATCH timer;
    sw_start(&timer);
    primes = model.function(n); // Run time
    sw_stop(&timer);
    elapsed_seconds = timer.elapsed_sec; // use cached elapsed time from sw_stop

    char n_str[32];
    snprintf(n_str, sizeof(n_str), "%d^%d", limit.base, limit.exp);
    printf("| %-16s", n_str);
    printf("| %-16d", primes->count);
    printf("| %-16" PRIu64, primes->array[primes->count - 1]);
    printf("| %-16f\n", elapsed_seconds); // time in seconds
    fflush(stdout);

    ui64_free(&primes);
    return (size_t)(elapsed_seconds * 1000000); // time in microseconds;
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
    fflush(stdout);
}

/**
 * @brief Benchmarks the performance of different sieve algorithms over a range of exponents.
 *
 * This function measures the execution time of sieve algorithms in SIEVE_MODELS for limits 10^4 to 10^10.
 * The results are printed and optionally saved to a file.
 *
 * @param save_results A flag indicating whether to save the results to a file named by timestamp in the output directory.
 */
void BENCHMARK_SIEVE_MODELS(int save_results)
{
    int times_array[models_count][32];
    SIEVE_LIMIT limits_array[32];
    int tests_count = 0;
    // define limits: 10^4 to 10^10
    for (int exp = 4; exp <= 10; exp++)
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
