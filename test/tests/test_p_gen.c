#include <test_api.h>

#include <openssl/bn.h>

/**
 * @brief A wrapper function to generate a random prime number using
 * the iZ_next_prime function.
 *
 * @param p The mpz_t variable to store the generated prime number.
 * @param bit_size The bit size of the prime number to be generated.
 */
int iZ_random_next_prime(mpz_t p, int bit_size)
{
    // Check if the bit size is within the valid range
    bit_size = MAX(bit_size, 10);

    // Set the initial random number within the magnitude range
    mpz_t base;
    mpz_init(base);

    // initialize the random state
    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state); // seed random state

    // Generate a random number in the given range
    mpz_urandomb(base, state, bit_size);

    // Find the next prime number after the random base
    int found = iZ_next_prime(p, base, 1);

    // cleanup
    mpz_clear(base);
    gmp_randclear(state);

    return found;
}

/**
 * @brief A wrapper function to generate a random prime number using
 * the GMP's mpz_nextprime function.
 *
 * @param p The mpz_t variable to store the generated prime number.
 * @param bit_size The bit size of the prime number to be generated.
 */
int gmp_random_next_prime(mpz_t p, int bit_size)
{
    bit_size = MAX(bit_size, 10);

    // Set the initial random number within the magnitude range
    mpz_t base;
    mpz_init(base);

    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state); // seed random state

    // Generate a random number in the given range
    mpz_urandomb(base, state, bit_size);

    // Find the next prime number after the random base
    mpz_nextprime(p, base);

    mpz_clear(base);
    gmp_randclear(state);

    return 1;
}

// ========================================================================
// * Testing iZ_next_prime Function
// ========================================================================

// tests for iZ_next_prime function against GMP's mpz_nextprime for same base.
// this test passes if both functions return the same prime for various bit sizes.
int TEST_iZ_next_prime(int verbose)
{
    print_test_fn_header("iZ_next_prime");
    printf("Comparing iZ_next_prime results with GMP's mpz_nextprime for the same base of various bit sizes...\n");

    int failed_tests = 0;

    // Test bit sizes [512, 1024, 2048, 4096]
    int bit_sizes[] = {512, 1024, 2048, 4096};
    int num_tests = sizeof(bit_sizes) / sizeof(bit_sizes[0]);

    for (int i = 1; i <= num_tests; i++)
    {
        int bit_size = bit_sizes[i - 1];

        // initialize random state
        gmp_randstate_t state;
        gmp_randinit_default(state);
        gmp_seed_randstate(state);

        // generate random base
        mpz_t base;
        mpz_init(base);
        mpz_urandomb(base, state, bit_size);

        // find next prime using iZ_next_prime
        mpz_t iz_prime;
        mpz_init(iz_prime);
        int iz_found = iZ_next_prime(iz_prime, base, 1);

        // find next prime using GMP's mpz_nextprime
        mpz_t gmp_prime;
        mpz_init(gmp_prime);
        mpz_nextprime(gmp_prime, base);

        // compare results
        if (iz_found)
        {
            if (mpz_cmp(iz_prime, gmp_prime) != 0)
            {
                failed_tests++;
                if (verbose)
                {
                    printf("[%d] Test Failed for bit size %d\n", i, bit_size);
                    printf("Base: %s\n", mpz_get_str(NULL, 10, base));
                    printf("iZ_next_prime: %s\n", mpz_get_str(NULL, 10, iz_prime));
                    printf("GMP mpz_nextprime: %s\n", mpz_get_str(NULL, 10, gmp_prime));
                }
            }
            else
            {
                if (verbose)
                    printf("[%d] Test Passed for bit size %d\n", i, bit_size);
            }
        }
        else
        {
            failed_tests++;
            if (verbose)
            {
                printf("[%d] Test Failed: iZ_next_prime did not find a prime.\n", i);
            }
        }

        // cleanup
        mpz_clears(base, iz_prime, gmp_prime, NULL);
        gmp_randclear(state);
    }

    printf("\n\n");
    print_line(60, '*');
    if (failed_tests == 0 && verbose)
    {
        printf("[SUCCESS] All iZ_next_prime tests passed! ^_^\n");
    }
    else if (failed_tests > 0 && verbose)
    {
        printf("[FAILURE] %d iZ_next_prime tests failed :\\\n", failed_tests);
    }
    print_line(60, '*');

    return (failed_tests == 0) ? 1 : 0;
}

// ========================================================================
// * Testing iZm Random Prime Generators
// ========================================================================

// this tests pass if the generated prime is indeed prime.

int TEST_vy_random_prime(int verbose)
{
    print_test_fn_header("vy_random_prime");
    printf("Testing vy_random_prime for various bit sizes and checking primality of results...\n");

    int failed_tests = 0;
    int bit_size[] = {512, 1024, 2048, 4096};
    int test_rounds = 4;

    for (int i = 1; i <= test_rounds; i++)
    {
        mpz_t p;
        mpz_init(p);
        int iz_found = vy_random_prime(p, bit_size[i - 1], 1);
        int iz_is_prime = mpz_probab_prime_p(p, MR_ROUNDS);
        if (iz_found && iz_is_prime)
        {
            if (verbose)
                printf("[%d] vy_random_prime: Test Passed for bit size %d\n", i, bit_size[i - 1]);
        }
        else
        {
            failed_tests++;
            if (verbose)
            {
                printf("[%d] vy_random_prime: Test Failed for bit size %d\n", i, bit_size[i - 1]);
                printf("Generated p: %s\n", mpz_get_str(NULL, 10, p));
            }
        }
        mpz_clear(p);
    }
    printf("\n\n");
    print_line(60, '*');
    if (failed_tests == 0 && verbose)
    {
        printf("[SUCCESS] All vy_random_prime tests passed! ^_^\n");
    }
    else if (failed_tests > 0 && verbose)
    {
        printf("[FAILURE] %d vy_random_prime tests failed :\\\n", failed_tests);
    }
    print_line(60, '*');

    return (failed_tests == 0) ? 1 : 0;
}

int TEST_vx_random_prime(int verbose)
{
    print_test_fn_header("vx_random_prime");
    printf("Testing vx_random_prime for various bit sizes and checking primality of results...\n");

    int failed_tests = 0;
    int bit_size[] = {512, 1024, 2048, 4096};
    int test_rounds = 4;

    for (int i = 1; i <= test_rounds; i++)
    {
        mpz_t p;
        mpz_init(p);
        int iz_found = vx_random_prime(p, bit_size[i - 1], 1);
        int iz_is_prime = mpz_probab_prime_p(p, MR_ROUNDS);
        if (iz_found && iz_is_prime)
        {
            if (verbose)
                printf("[%d] vx_random_prime: Test Passed for bit size %d\n", i, bit_size[i - 1]);
        }
        else
        {
            failed_tests++;
            if (verbose)
            {
                printf("[%d] vx_random_prime: Test Failed for bit size %d\n", i, bit_size[i - 1]);
                printf("Generated p: %s\n", mpz_get_str(NULL, 10, p));
            }
        }
        mpz_clear(p);
    }
    printf("\n\n");
    print_line(60, '*');
    if (failed_tests == 0 && verbose)
    {
        printf("[SUCCESS] All vx_random_prime tests passed! ^_^\n");
    }
    else if (failed_tests > 0 && verbose)
    {
        printf("[FAILURE] %d vx_random_prime tests failed :\\\n", failed_tests);
    }
    print_line(60, '*');

    return (failed_tests == 0) ? 1 : 0;
}

// ========================================================================
// * Benchmarking Random Prime Generation Algorithms
// ========================================================================

typedef enum
{
    VYp,  // vy_random_prime
    VYp4, // vy_random_prime with 4 cores
    VYp8, // vy_random_prime with 8 cores
    VXp,  // vx_random_prime
    VXp4, // vx_random_prime with 4 cores
    VXp8, // vx_random_prime with 8 cores
    iZn,  // iZ_random_next_prime
    GMPn, // gmp_random_next_prime
    SSLp, // BN_generate_prime_ex
} P_GEN_ALGORITHM;

typedef struct
{
    P_GEN_ALGORITHM algorithm;
    char algorithm_name[32];
    int bit_size;
    int cores_num;
    char *p_str;
    double time;
} GenResult;

static void print_benchmark_results(GenResult *results, int test_rounds)
{
    if (test_rounds <= 0)
        return;

    // print results
    print_centered_text("Benchmark Results", 60, '=');
    printf("Algorithm          : %s\n", results[0].algorithm_name);
    printf("Bit Size           : %d\n", results[0].bit_size);
    printf("Cores              : %d\n", results[0].cores_num);

    printf("Execution Times (s): [%.6f", results[0].time);
    for (int i = 1; i < test_rounds; i++)
    {
        printf(", %.6f", results[i].time);
    }
    printf("]\n");
    double total_time = 0;
    for (int i = 0; i < test_rounds; i++)
    {
        total_time += results[i].time;
    }
    printf("Average Time (s)   : %.6f\n", total_time / test_rounds);
    print_line(60, '=');
}

GenResult *BENCHMARK_vy_random_prime(int bit_size, int test_rounds, int cores_num)
{
    GenResult *results = malloc(sizeof(GenResult) * test_rounds);

    STOPWATCH timer;

    for (int i = 0; i < test_rounds; i++)
    {
        GenResult result;
        mpz_t p;
        mpz_init(p);

        sw_start(&timer);
        vy_random_prime(p, bit_size, cores_num);
        sw_stop(&timer);

        result.algorithm = VYp;
        snprintf(result.algorithm_name, sizeof(result.algorithm_name), "vy_random_prime");
        result.cores_num = cores_num;
        result.bit_size = bit_size;
        result.p_str = mpz_get_str(NULL, 10, p);
        result.time = timer.elapsed_sec;
        results[i] = result;

        mpz_clear(p);
    }

    // print results
    print_benchmark_results(results, test_rounds);
    fflush(stdout);

    return results;
}

GenResult *BENCHMARK_vx_random_prime(int bit_size, int test_rounds, int cores_num)
{
    GenResult *results = malloc(sizeof(GenResult) * test_rounds);

    STOPWATCH timer;

    for (int i = 0; i < test_rounds; i++)
    {
        GenResult result;
        mpz_t p;
        mpz_init(p);

        sw_start(&timer);
        vx_random_prime(p, bit_size, cores_num);
        sw_stop(&timer);

        result.algorithm = VXp;
        snprintf(result.algorithm_name, sizeof(result.algorithm_name), "vx_random_prime");
        result.cores_num = cores_num;
        result.bit_size = bit_size;
        result.p_str = mpz_get_str(NULL, 10, p);
        result.time = timer.elapsed_sec;
        results[i] = result;

        mpz_clear(p);
    }

    // print results
    print_benchmark_results(results, test_rounds);
    fflush(stdout);

    return results;
}

GenResult *BENCHMARK_iZ_random_next_prime(int bit_size, int test_rounds)
{
    GenResult *results = malloc(sizeof(GenResult) * test_rounds);

    STOPWATCH timer;

    for (int i = 0; i < test_rounds; i++)
    {
        GenResult result;
        mpz_t p;
        mpz_init(p);

        sw_start(&timer);
        iZ_random_next_prime(p, bit_size);
        sw_stop(&timer);

        result.algorithm = iZn;
        snprintf(result.algorithm_name, sizeof(result.algorithm_name), "iZ_random_next_prime");
        result.cores_num = 1;
        result.bit_size = bit_size;
        result.p_str = mpz_get_str(NULL, 10, p);
        result.time = timer.elapsed_sec;
        results[i] = result;

        mpz_clear(p);
    }

    // print results
    print_benchmark_results(results, test_rounds);
    fflush(stdout);

    return results;
}

GenResult *BENCHMARK_gmp_random_next_prime(int bit_size, int test_rounds)
{
    GenResult *results = malloc(sizeof(GenResult) * test_rounds);

    STOPWATCH timer;

    for (int i = 0; i < test_rounds; i++)
    {
        GenResult result;
        mpz_t p;
        mpz_init(p);

        sw_start(&timer);
        gmp_random_next_prime(p, bit_size);
        sw_stop(&timer);

        result.algorithm = iZn;
        snprintf(result.algorithm_name, sizeof(result.algorithm_name), "gmp_random_next_prime");
        result.cores_num = 1;
        result.bit_size = bit_size;
        result.p_str = mpz_get_str(NULL, 10, p);
        result.time = timer.elapsed_sec;
        results[i] = result;

        mpz_clear(p);
    }

    // print results
    print_benchmark_results(results, test_rounds);
    fflush(stdout);

    return results;
}

GenResult *BENCHMARK_BN_generate_prime_ex(int bit_size, int test_rounds)
{
    GenResult *results = malloc(sizeof(GenResult) * test_rounds);

    STOPWATCH timer;

    for (int i = 0; i < test_rounds; i++)
    {
        GenResult result;
        // OpenSSL algorithm: use BN_generate_prime_ex
        BIGNUM *p = BN_new();

        sw_start(&timer);
        BN_generate_prime_ex(p, bit_size, 0, NULL, NULL, NULL);
        sw_stop(&timer);

        result.algorithm = iZn;
        snprintf(result.algorithm_name, sizeof(result.algorithm_name), "BN_generate_prime_ex");
        result.cores_num = 1;
        result.bit_size = bit_size;
        result.p_str = BN_bn2dec(p);
        result.time = timer.elapsed_sec;
        results[i] = result;

        BN_free(p);
    }

    // print results
    print_benchmark_results(results, test_rounds);
    fflush(stdout);

    return results;
}

// ===============================

int BENCHMARK_P_GEN_ALGORITHMS(int bit_size, int test_rounds, int save_results)
{
    P_GEN_ALGORITHM algorithms[] = {VYp, VYp4, VYp8, VXp, VXp4, VXp8, iZn, GMPn, SSLp};
    int alg_num = sizeof(algorithms) / sizeof(algorithms[0]);
    GenResult *all_results[alg_num];
    int total_tests = 0;
    int ok = 1;

    // * Testing vy_random_prime
    GenResult *vy_results = BENCHMARK_vy_random_prime(bit_size, test_rounds, 1);
    all_results[total_tests++] = vy_results;

    GenResult *vy4_results = BENCHMARK_vy_random_prime(bit_size, test_rounds, 4);
    all_results[total_tests++] = vy4_results;

    GenResult *vy8_results = BENCHMARK_vy_random_prime(bit_size, test_rounds, 8);
    all_results[total_tests++] = vy8_results;

    // * Testing vx_random_prime
    GenResult *vx_results = BENCHMARK_vx_random_prime(bit_size, test_rounds, 1);
    all_results[total_tests++] = vx_results;

    GenResult *vx4_results = BENCHMARK_vx_random_prime(bit_size, test_rounds, 4);
    all_results[total_tests++] = vx4_results;

    GenResult *vx8_results = BENCHMARK_vx_random_prime(bit_size, test_rounds, 8);
    all_results[total_tests++] = vx8_results;

    // * Testing iZ_random_next_prime
    GenResult *iZ_results = BENCHMARK_iZ_random_next_prime(bit_size, test_rounds);
    all_results[total_tests++] = iZ_results;

    // * Testing gmp_random_next_prime
    GenResult *gmp_results = BENCHMARK_gmp_random_next_prime(bit_size, test_rounds);
    all_results[total_tests++] = gmp_results;

    // * Testing BN_generate_prime_ex
    GenResult *ssl_results = BENCHMARK_BN_generate_prime_ex(bit_size, test_rounds);
    all_results[total_tests++] = ssl_results;

    if (save_results)
    {
        if (iz_platform_create_dir(DIR_output) != 0)
        {
            log_error("Failed to create output directory in BENCHMARK_P_GEN_ALGORITHMS: %s", DIR_output);
            ok = 0;
            goto cleanup;
        }

        // Get the current timestamp for file naming.
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp) - 1, "%d%H%M%S", t);

        // Build the output file path.
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/p_gen_%s.txt", DIR_output, timestamp);

        FILE *file = fopen(file_path, "w");
        if (file == NULL)
        {
            log_error("Failed to open file in TEST_P_GEN_ALGORITHMS: %s", file_path);
            ok = 0;
            goto cleanup;
        }

        for (int i = 0; i < total_tests; i++)
        {
            GenResult *result = all_results[i];

            fprintf(file, "Algorithm: %s\n", result->algorithm_name);
            fprintf(file, "Bit Size: %d\n", result->bit_size);
            fprintf(file, "Cores: %d\n", result->cores_num);
            fprintf(file, "Primes Results:\n");
            for (int j = 0; j < test_rounds; j++)
            {
                fprintf(file, "[%d]: %s\n", j + 1, result[j].p_str);
            }

            double total_time = result[0].time;
            fprintf(file, "Execution Times (s): [%.6f", result[0].time);
            for (int j = 1; j < test_rounds; j++)
            {
                fprintf(file, ", %.6f", result[j].time);
                total_time += result[j].time;
            }
            fprintf(file, "]\n");
            fprintf(file, "Average Time: %.6f seconds\n", total_time / test_rounds);
            fprintf(file, "\n");
        }

        fclose(file);
        printf("Results saved to %s\n\n", file_path);
        printf("RESULTS_FILE: %s\n", file_path);
        fflush(stdout);
    }

cleanup:
    // Free all results
    for (int i = 0; i < total_tests; i++)
    {
        free(all_results[i]->p_str);
        free(all_results[i]);
    }

    return ok;
}
