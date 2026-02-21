#include <test_api.h>
#include <inttypes.h>

// =======================================================================
// * Testing SiZ_stream
// =======================================================================
int TEST_SiZ_stream(int verbose)
{
    int failed_tests = 0;

    uint64_t test_count = 0;
    double elapsed_seconds = 0;
    STOPWATCH timer;

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

    sw_start(&timer);
    test_count = SiZ_stream(&input_range);
    sw_stop(&timer);
    elapsed_seconds = sw_elapsed_seconds(&timer);

    // Check result
    if (test_count != expected_count)
        failed_tests++;

    printf("Test 1: Streaming primes in range [%s:%s]\n", input_range.start, mpz_get_str(NULL, 10, end_num));
    if (verbose)
    {
        printf("%-32s: %" PRIu64 "\n", "Expected primes count", expected_count);
        printf("%-32s: %" PRIu64 "\n", "Result primes count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        printf("%-32s: %s\n", "Output File", input_range.filepath);
    }
    else
    {
        if (test_count != expected_count)
        {
            printf("Expected primes count: %" PRIu64 ", Got: %" PRIu64 "\n", expected_count, test_count);
        }
    }
    // ===================================
    // Test 2
    input_range.start = "1000000000000";
    expected_count = 36249;
    input_range.filepath = "./output/SiZ_stream_test2.txt";
    mpz_set_str(end_num, input_range.start, 10);
    mpz_add_ui(end_num, end_num, test_range);

    print_line(60, '=');
    printf("Test 2: Streaming primes in range [%s:%s]\n", input_range.start, mpz_get_str(NULL, 10, end_num));
    print_line(60, '=');

    sw_start(&timer);
    test_count = SiZ_stream(&input_range);
    sw_stop(&timer);
    elapsed_seconds = sw_elapsed_seconds(&timer);

    // Check result
    if (test_count != expected_count)
        failed_tests++;

    if (verbose)
    {
        printf("%-32s: %" PRIu64 "\n", "Expected primes count", expected_count);
        printf("%-32s: %" PRIu64 "\n", "Result primes count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        printf("%-32s: %s\n", "Output File", input_range.filepath);
    }
    else
    {
        if (test_count != expected_count)
        {
            printf("Expected primes count: %" PRIu64 ", Got: %" PRIu64 "\n", expected_count, test_count);
        }
    }

    printf("\n");
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
int TEST_SiZ_count(int verbose)
{
    int result = 1;

    int cores_num = MAX_CORES;
    uint64_t test_count = 0;
    double elapsed_seconds = 0;
    STOPWATCH timer;

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

    sw_start(&timer);
    test_count = SiZ_count(&input_range, 1);
    sw_stop(&timer);
    elapsed_seconds = sw_elapsed_seconds(&timer);

    uint64_t expected_count = 50847534;
    result = test_count == expected_count;

    if (verbose)
    {
        printf("%-32s: %" PRIu64 "\n", "Expected prime count", expected_count);
        printf("%-32s: %" PRIu64 "\n", "Result prime count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        fflush(stdout);
    }

    // ===========
    // * Test 2: 0 to 10^9 multi-core
    print_line(30, '=');
    printf("Test 2: Counting primes in range [%s:%s] using %d cores\n", input_range.start, mpz_get_str(NULL, 10, end_num), cores_num);
    fflush(stdout);

    sw_start(&timer);
    test_count = SiZ_count(&input_range, cores_num);
    sw_stop(&timer);
    elapsed_seconds = sw_elapsed_seconds(&timer);

    mpz_set_str(end_num, input_range.start, 10);
    mpz_add_ui(end_num, end_num, interval);

    expected_count = 50847534;
    result = test_count == expected_count;

    if (verbose)
    {
        printf("%-32s: %" PRIu64 "\n", "Expected prime count", expected_count);
        printf("%-32s: %" PRIu64 "\n", "Result prime count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        fflush(stdout);
    }
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

// =======================================================================
// * Benchmark SiZ_count
// =======================================================================

// benchmarking SiZ_count for 10^9 range starting from 10^10, 10^20, ..., 10^100 using max cores
void BENCHMARK_SiZ_count(int save_results)
{
    int cores_num = MAX_CORES;

    printf("Test range is 10^9 starting after [10^10, 10^20, ..., 10^100] using %d cores\n", cores_num);
    print_line(60, '=');
    fflush(stdout);

    uint64_t test_count = 0;
    INPUT_SIEVE_RANGE input_range = {
        .start = "0",
        .range = 1000000000ULL,
        .mr_rounds = MR_ROUNDS,
        .filepath = NULL,
    };

    char start_str[128];
    mpz_t start_num, end_num;
    mpz_init(start_num);
    mpz_init(end_num);

    int exponents[16];
    uint64_t counts[16];
    double times[16];
    int tests_count = 0;

    double elapsed_seconds = 0;
    STOPWATCH timer;

    // loop over input_range.start in 10^10, 10^20, ..., 10^100
    // for (int exp = 10; exp <= 20; exp += 10) // quick test
    for (int exp = 10; exp <= 100; exp += 10)
    {
        mpz_ui_pow_ui(start_num, 10, exp);
        mpz_get_str(start_str, 10, start_num);
        input_range.start = start_str;
        mpz_set(end_num, start_num);
        mpz_add_ui(end_num, end_num, input_range.range);

        sw_start(&timer);
        test_count = SiZ_count(&input_range, cores_num);
        sw_stop(&timer);
        elapsed_seconds = sw_elapsed_seconds(&timer);

        printf("%-32s: [10^%d, 10^%d + 10^9]\n", "Test Range", exp, exp);
        printf("%-32s: %" PRIu64 "\n", "Primes count", test_count);
        printf("%-32s: %f\n", "Execution time (s)", elapsed_seconds);
        print_line(60, '=');
        fflush(stdout);

        exponents[tests_count] = exp;
        counts[tests_count] = test_count;
        times[tests_count] = elapsed_seconds;
        tests_count++;
    }

    if (save_results)
    {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp) - 1, "%d%H%M%S", t);

        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/SiZ_count_%s.txt", DIR_output, timestamp);

        FILE *file = fopen(file_path, "w");
        if (file == NULL)
        {
            log_error("Failed to open file in BENCHMARK_SiZ_count: %s", file_path);
        }
        else
        {
            fprintf(file, "Benchmark: SiZ_count\n");
            fprintf(file, "Cores: %d\n", cores_num);
            fprintf(file, "Range Size: 10^9\n");
            fprintf(file, "Results:\n");

            for (int i = 0; i < tests_count; i++)
            {
                fprintf(file, "Start=10^%d, Primes Count=%" PRIu64 ", Execution Time (s)=%.6f\n",
                        exponents[i], counts[i], times[i]);
            }

            fclose(file);
            printf("Results saved to %s\n", file_path);
            fflush(stdout);
        }
    }

    mpz_clear(start_num);
    mpz_clear(end_num);
}
