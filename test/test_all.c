/**
 * @file test_all.c
 * @brief Test runner for unit, integration, and benchmark suites.
 */

#include <test_api.h>

static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("\n");
    printf("Test selection (default: --all):\n");
    printf("  --all                 Run unit + integration tests\n");
    printf("  --unit                Run unit tests\n");
    printf("  --integration         Run integration tests\n");
    printf("Benchmarking options:\n");
    printf("  --benchmark           Run sieve benchmarks (alias for --benchmark-p-sieve)\n");
    printf("  --benchmark-p-sieve   Run prime sieve model benchmarks\n");
    printf("  --benchmark-siz-count Run SiZ_count benchmark (10^9 windows from 10^10..10^100)\n");
    printf("  --benchmark-p-gen     Run random prime generation benchmarks\n");
    printf("\n");
    printf("Output/options:\n");
    printf("  -v, --verbose         Verbose test output\n");
    printf("  --save-results        Save benchmark results (if supported)\n");
    printf("  --save_results        Alias for --save-results\n");
    printf("  --plot                Generate plots for benchmark results (if supported)\n");
    printf("  -h, --help            Show this help\n");
}

typedef struct
{
    int verbose;
    int save_results;
    int run_units;
    int run_integrations;
    int run_benchmarks_sieve;
    int run_benchmark_siz_count;
    int run_benchmarks_p_gen;
} RUNNER_OPTIONS;

typedef void (*RUNNER_FLAG_HANDLER)(RUNNER_OPTIONS *opts);

typedef struct
{
    const char *flag;
    RUNNER_FLAG_HANDLER handler;
} RUNNER_FLAG_SPEC;

static void opt_run_all(RUNNER_OPTIONS *opts)
{
    opts->run_units = 1;
    opts->run_integrations = 1;
}

static void opt_run_units(RUNNER_OPTIONS *opts)
{
    opts->run_units = 1;
}

static void opt_run_integrations(RUNNER_OPTIONS *opts)
{
    opts->run_integrations = 1;
}

static void opt_run_benchmark_sieve(RUNNER_OPTIONS *opts)
{
    opts->run_benchmarks_sieve = 1;
}

static void opt_run_benchmark_siz_count(RUNNER_OPTIONS *opts)
{
    opts->run_benchmark_siz_count = 1;
}

static void opt_run_benchmark_p_gen(RUNNER_OPTIONS *opts)
{
    opts->run_benchmarks_p_gen = 1;
}

static void opt_save_results(RUNNER_OPTIONS *opts)
{
    opts->save_results = 1;
}

static void opt_verbose(RUNNER_OPTIONS *opts)
{
    opts->verbose = 1;
}

static const RUNNER_FLAG_SPEC k_flag_specs[] = {
    {"--all", opt_run_all},
    {"--unit", opt_run_units},
    {"--integration", opt_run_integrations},
    {"--benchmark", opt_run_benchmark_sieve},
    {"--benchmark-p-sieve", opt_run_benchmark_sieve},
    {"--benchmark-siz-count", opt_run_benchmark_siz_count},
    {"--benchmark-p-gen", opt_run_benchmark_p_gen},
    {"--save-results", opt_save_results},
    {"--save_results", opt_save_results},
    {"-v", opt_verbose},
    {"--verbose", opt_verbose},
};

static int parse_command(int argc, char **argv, RUNNER_OPTIONS *opts)
{
    memset(opts, 0, sizeof(*opts));

    if (argc == 1)
    {
        opt_run_all(opts);
        return 0;
    }

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            print_usage(argv[0]);
            return 1;
        }

        int matched = 0;
        size_t specs_count = sizeof(k_flag_specs) / sizeof(k_flag_specs[0]);
        for (size_t j = 0; j < specs_count; j++)
        {
            if (strcmp(arg, k_flag_specs[j].flag) == 0)
            {
                k_flag_specs[j].handler(opts);
                matched = 1;
                break;
            }
        }

        if (!matched)
        {
            fprintf(stderr, "Unknown option: %s\n\n", arg);
            print_usage(argv[0]);
            return -1;
        }
    }

    if (!opts->run_units && !opts->run_integrations && !opts->run_benchmarks_sieve &&
        !opts->run_benchmark_siz_count && !opts->run_benchmarks_p_gen)
    {
        opt_run_all(opts);
    }

    return 0;
}

int RUN_TEST_UNITS(int verbose)
{
    int result = 1;

    print_centered_text(" Running All Unit Module Tests ", 60, '=');
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

    // * Run UTILS tests
    printf("\n\n");
    result = TEST_UTILS(verbose);
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
    print_centered_text(" End of Unit Module Tests ", 60, '=');

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
    print_centered_text(" End of Integration Tests ", 60, '=');

    return failed_tests == 0;
}

void RUN_BENCHMARK_SIEVE_MODELS(int save_results)
{
    print_centered_text(" Benchmarking All Sieve Models ", 60, '=');
    printf("\n");
    BENCHMARK_SIEVE_MODELS(save_results);
    printf("\n\n");
    print_centered_text(" End of Sieve Models Benchmarking ", 60, '=');
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
    print_centered_text(" End of Random Prime Generation Algorithms Benchmarking ", 60, '=');
}

void RUN_BENCHMARK_SiZ_COUNT(int save_results)
{
    print_centered_text(" Benchmarking SiZ_count ", 60, '=');
    printf("\n");
    BENCHMARK_SiZ_count(save_results);
    printf("\n");
    print_centered_text(" End of SiZ_count Benchmarking ", 60, '=');
}

int main(int argc, char **argv)
{
    // Set log level to debug to log implementation warnings and errors
    log_set_log_level(LOG_DEBUG);

    RUNNER_OPTIONS opts;
    int parse_status = parse_command(argc, argv, &opts);
    if (parse_status > 0)
        return 0;
    if (parse_status < 0)
        return 2;

    int ok = 1;
    if (opts.run_units)
        ok = ok && RUN_TEST_UNITS(opts.verbose);
    if (opts.run_integrations)
        ok = ok && RUN_TEST_INTEGRATIONS(opts.verbose);
    if (opts.run_benchmarks_sieve)
        RUN_BENCHMARK_SIEVE_MODELS(opts.save_results);
    if (opts.run_benchmark_siz_count)
        RUN_BENCHMARK_SiZ_COUNT(opts.save_results);
    if (opts.run_benchmarks_p_gen)
        RUN_BENCHMARK_P_GEN_ALGORITHMS(opts.save_results);

    return ok ? 0 : 1;
}
