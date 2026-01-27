#include <test_api.h>

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

    return result;
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

    return result;
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
    // RUN_BENCHMARK_P_GEN_ALGORITHMS(save_results);
}

int main(void)
{
    // Set log level to debug to log implementation errors
    log_set_log_level(LOG_DEBUG);

    int verbose = 1;
    int save_results = 0;

    // * Run all tests and benchmarks
    // RUN_ALL_TESTS_AND_BENCHMARKS(verbose, save_results);

    // * Unit Tests:
    // run all units tests or uncomment individual tests below
    // RUN_TEST_UNITS(verbose); // * run all unit tests
    // TEST_BITMAP(verbose);
    // TEST_UI16_ARRAY(verbose);
    // TEST_UI32_ARRAY(verbose);
    // TEST_UI64_ARRAY(verbose);
    // TEST_IZM(verbose);
    // TEST_VX_SEG(verbose);

    // * Integration Tests:
    // run all integrations tests or uncomment individual tests below
    // RUN_TEST_INTEGRATIONS(verbose); // * run all integration tests
    // TEST_SIEVE_MODELS_INTEGRITY(verbose);
    // TEST_SiZ_stream(verbose);
    // TEST_SiZ_count(verbose);
    // TEST_iZ_next_prime(verbose);
    // TEST_vy_random_prime(verbose);
    // TEST_vx_random_prime(verbose);

    // * Benchmarking Sieve Algorithms:
    RUN_BENCHMARK_SIEVE_MODELS(save_results);

    // * Benchmarking Random Prime Generation Algorithms:
    // RUN_BENCHMARK_P_GEN_ALGORITHMS(save_results);

    return 0;
}
