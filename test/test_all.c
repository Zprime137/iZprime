/**
 * @file test_all.c
 * @brief Test runner for unit, integration, and benchmark suites.
 */

#include <test_api.h>
#include <string.h>

int RUN_TEST_UNITS(int verbose)
{
    int result = 1;

    print_centered_text(" Running All Module Tests ", 60, '=');
    printf("\n");

    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;

    // * Run BITMAP tests
    result = TEST_BITMAP(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run UI16_ARRAY tests
    printf("\n\n");
    result = TEST_UI16_ARRAY(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run UI32_ARRAY tests
    printf("\n\n");
    result = TEST_UI32_ARRAY(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run UI64_ARRAY tests
    printf("\n\n");
    result = TEST_UI64_ARRAY(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run IZM tests
    printf("\n\n");
    result = TEST_IZM(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run VX_SEG tests
    printf("\n\n");
    result = TEST_VX_SEG(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Print overall summary
    printf("\n\n");
    print_line(60, '*');
    printf("OVERALL UNITS TEST SUMMARY\n");
    print_line(60, '-');
    printf("%-32s: %d\n", "Total Modules Tested", total_tests);
    printf("%-32s: %d\n", "Modules Passed", passed_tests);
    printf("%-32s: %d\n", "Modules Failed", failed_tests);
    printf("%-32s: %.1f%%\n", "Success Rate", (passed_tests * 100.0) / total_tests);
    print_line(60, '-');

    if (failed_tests == 0)
    {
        printf("[SUCCESS] ALL MODULE TESTS PASSED! ^_^\n");
    }
    else
    {
        printf("[FAILURE] SOME MODULE TESTS FAILED :\\\n");
        printf("Please check the logs for details.\n");
    }
    print_line(60, '*');

    printf("\n\n");
    print_centered_text(" Tests Completed ", 60, '=');

    return failed_tests == 0;
}

int RUN_TEST_INTEGRATIONS(int verbose)
{
    int result = 1;

    print_centered_text(" Running All Integration Tests ", 60, '=');
    printf("\n");
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;

    // * Run SIEVE INTEGRITY tests
    result = TEST_SIEVE_MODELS_INTEGRITY(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run TEST_SiZ_stream tests
    printf("\n\n");
    result = TEST_SiZ_stream(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run SiZ_count tests
    printf("\n\n");
    result = TEST_SiZ_count(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run iZ_next_prime tests
    printf("\n\n");
    result = TEST_iZ_next_prime(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run vy_random_prime tests
    printf("\n\n");
    result = TEST_vy_random_prime(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Run vx_random_prime tests
    printf("\n\n");
    result = TEST_vx_random_prime(verbose);
    total_tests++;
    if (result)
        passed_tests++;
    else
        failed_tests++;

    // * Print overall summary
    printf("\n\n");
    print_line(60, '*');
    printf("OVERALL INTEGRATIONS TEST SUMMARY\n");
    print_line(60, '-');
    printf("Total Integration Tests: %d\n", total_tests);
    printf("Integration Tests Passed: %d\n", passed_tests);
    printf("Integration Tests Failed: %d\n", failed_tests);
    printf("Success Rate:             %.1f%%\n", (passed_tests * 100.0) / total_tests);
    print_line(60, '-');
    if (failed_tests == 0)
    {
        printf("[SUCCESS] ALL INTEGRATION TESTS PASSED!\n");
    }
    else
    {
        printf("[FAILURE] SOME INTEGRATION TESTS FAILED :\\\n");
        if (!verbose)
            printf("Run with verbose=1 for detailed output.\n");
    }
    print_line(60, '*');
    printf("\n");
    print_centered_text(" Integration Tests Completed ", 60, '=');

    return failed_tests == 0;
}

static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("\n");
    printf("Test selection (default: --unit --integration):\n");
    printf("  --all            Run unit + integration tests\n");
    printf("  --unit           Run unit tests\n");
    printf("  --integration    Run integration tests\n");
    printf("  --benchmark      Run sieve benchmarks (alias for --benchmark-p-sieve)\n");
    printf("  --benchmark-p-sieve  Run prime sieve model benchmarks\n");
    printf("  --benchmark-p-gen    Run random prime generation benchmarks\n");
    printf("\n");
    printf("Output/options:\n");
    printf("  -v, --verbose    Verbose test output\n");
    printf("  --save-results   Save benchmark results (if supported)\n");
    printf("  -h, --help       Show this help\n");
}

void RUN_BENCHMARK_SIEVE_MODELS(int save_results)
{
    print_centered_text(" Benchmarking All Sieve Models ", 60, '=');
    printf("\n");
    BENCHMARK_SIEVE_MODELS(save_results);
    printf("\n\n");
    print_centered_text(" Benchmarking Completed ", 60, '=');
}

void RUN_BENCHMARK_P_GEN_ALGORITHMS(int save_results)
{
    int bit_sizes[] = {1024, 2048, 4096};
    int count = sizeof(bit_sizes) / sizeof(bit_sizes[0]);
    int test_rounds = 5;

    print_centered_text(" Benchmarking Random Prime Generation Algorithms ", 60, '=');
    printf("\n\n");
    for (int i = 0; i < count; i++)
    {
        BENCHMARK_P_GEN_ALGORITHMS(bit_sizes[i], test_rounds, save_results);
        printf("\n\n");
    }
    print_centered_text(" Benchmarking Completed ", 60, '=');
}

void RUN_ALL_TESTS_AND_BENCHMARKS(int verbose, int save_results)
{
    // * Unit Tests
    RUN_TEST_UNITS(verbose);

    // * Integration Tests
    RUN_TEST_INTEGRATIONS(verbose);

    // * Benchmarking Sieve Models
    RUN_BENCHMARK_SIEVE_MODELS(save_results);

    // * Benchmarking Random Prime Generation Algorithms
    RUN_BENCHMARK_P_GEN_ALGORITHMS(save_results);
}

int main(int argc, char **argv)
{
    // Set log level to debug to log implementation errors
    log_set_log_level(LOG_DEBUG);

    int verbose = 0;
    int save_results = 0;

    int run_units = 0;
    int run_integrations = 0;
    int run_benchmarks_sieve = 0;
    int run_benchmarks_p_gen = 0;

    if (argc == 1)
    {
        run_units = 1;
        run_integrations = 1;
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            const char *arg = argv[i];

            if (strcmp(arg, "--all") == 0)
            {
                run_units = 1;
                run_integrations = 1;
            }
            else if (strcmp(arg, "--unit") == 0)
            {
                run_units = 1;
            }
            else if (strcmp(arg, "--integration") == 0)
            {
                run_integrations = 1;
            }
            else if (strcmp(arg, "--benchmark") == 0)
            {
                run_benchmarks_sieve = 1; // backward compatible default
            }
            else if (strcmp(arg, "--benchmark-p-sieve") == 0)
            {
                run_benchmarks_sieve = 1;
            }
            else if (strcmp(arg, "--benchmark-p-gen") == 0)
            {
                run_benchmarks_p_gen = 1;
            }
            else if (strcmp(arg, "--save-results") == 0)
            {
                save_results = 1;
            }
            else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0)
            {
                verbose = 1;
            }
            else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
            {
                print_usage(argv[0]);
                return 0;
            }
            else
            {
                fprintf(stderr, "Unknown option: %s\n\n", arg);
                print_usage(argv[0]);
                return 2;
            }
        }
    }

    if (!run_units && !run_integrations && !run_benchmarks_sieve && !run_benchmarks_p_gen)
    {
        run_units = 1;
        run_integrations = 1;
    }

    int ok = 1;
    if (run_units)
        ok = ok && RUN_TEST_UNITS(verbose);
    if (run_integrations)
        ok = ok && RUN_TEST_INTEGRATIONS(verbose);
    if (run_benchmarks_sieve)
        RUN_BENCHMARK_SIEVE_MODELS(save_results);
    if (run_benchmarks_p_gen)
        RUN_BENCHMARK_P_GEN_ALGORITHMS(save_results);

    return ok ? 0 : 1;
}
