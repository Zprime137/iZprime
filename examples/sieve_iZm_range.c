#include <api.h>

void example_sieve_iZm_stream(void)
{
}

void benchmark_sieve_iZm_range(void)
{
    int cores_num = get_cpu_cores_count();
    mpz_t end_range;
    mpz_init(end_range);
    struct timeval start, end;
    double elapsed_seconds;
    uint64_t test_count;
    int result;

    char start_str[128] = "0";
    // uint64_t test_range = 1000000000;
    // uint64_t expected_count = 50847534;
    uint64_t test_range = 1000000000000;
    uint64_t expected_count = 37607912018;
    mpz_set_str(end_range, start_str, 10);
    mpz_add_ui(end_range, end_range, test_range);

    INPUT_SIEVE_RANGE input_range = {
        .start = start_str,
        .range = test_range,
        .mr_rounds = MR_ROUNDS,
        .filepath = NULL,
    };

    gettimeofday(&start, NULL);
    // test_count = SiZ_stream(&input_range); // using single core
    test_count = SiZ_count(&input_range, cores_num); // using multiple cores
    gettimeofday(&end, NULL);
    elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    result = test_count == expected_count;

    print_line(30, '=');
    printf("Expected prime count: %-16llu\n", expected_count);
    printf("Result prime count:   %-16llu\n", test_count);
    printf("Execution time (s):   %-16f\n", elapsed_seconds);
    fflush(stdout);
}

int main(void)
{
    benchmark_sieve_iZm_range();

    return EXIT_SUCCESS;
}
