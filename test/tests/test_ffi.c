#include <test_api.h>
#include <izprime_ffi.h>

static uint64_t naive_prime_count_u64(uint64_t start, uint64_t range)
{
    uint64_t total = 0;
    mpz_t z;
    mpz_init(z);

    for (uint64_t i = 0; i < range; i++)
    {
        mpz_set_ui(z, start + i);
        if (mpz_cmp_ui(z, 2) >= 0 && mpz_probab_prime_p(z, 30) > 0)
            total++;
    }

    mpz_clear(z);
    return total;
}

int TEST_FFI(int verbose)
{
    char module_name[] = "FFI";
    int passed_tests = 0;
    int failed_tests = 0;
    int current_test_idx = 0;

    print_test_module_header(module_name);
    if (verbose)
        print_test_table_header();

    current_test_idx++;
    const char *version = izp_ffi_version();
    if (version && version[0] != '\0')
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "izp_ffi_version", "version=%s", version);
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "izp_ffi_version", "empty version string");
    }

    current_test_idx++;
    IZP_U64_BUFFER primes = {0};
    int status = izp_ffi_sieve_u64(IZP_SIEVE_SIZM, 1000, &primes);
    if (status == IZP_FFI_OK && primes.len == 168 && primes.data && primes.data[primes.len - 1] == 997)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "izp_ffi_sieve_u64", "count=%zu last=%" PRIu64, primes.len, primes.data[primes.len - 1]);
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "izp_ffi_sieve_u64", "status=%d err=%s", status, izp_ffi_last_error());
    }
    izp_ffi_free_u64_buffer(&primes);

    current_test_idx++;
    status = izp_ffi_sieve_u64(IZP_SIEVE_SOE, 1000000000001ULL, &primes);
    if (status == IZP_FFI_ERR_INVALID_ARG)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "izp_ffi_sieve_u64", "rejects n > 1e12");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "izp_ffi_sieve_u64", "status=%d err=%s", status, izp_ffi_last_error());
    }

    current_test_idx++;
    char *next_prime = NULL;
    status = izp_ffi_next_prime("100", 1, &next_prime);
    if (status == IZP_FFI_OK && next_prime && strcmp(next_prime, "101") == 0)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "izp_ffi_next_prime", "100 -> %s", next_prime);
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "izp_ffi_next_prime", "status=%d err=%s", status, izp_ffi_last_error());
    }
    izp_ffi_free_string(&next_prime);

    current_test_idx++;
    uint64_t ffi_count = 0;
    uint64_t expected_count = naive_prime_count_u64(1000, 80);
    status = izp_ffi_count_range("1000", 80, 30, 2, &ffi_count);
    if (status == IZP_FFI_OK && ffi_count == expected_count)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "izp_ffi_count_range", "count=%" PRIu64, ffi_count);
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "izp_ffi_count_range", "status=%d got=%" PRIu64 " expected=%" PRIu64 " err=%s", status, ffi_count, expected_count, izp_ffi_last_error());
    }

    current_test_idx++;
    create_dir(DIR_output);
    const char *stream_out = DIR_output "/ffi_stream_smoke.txt";
    uint64_t stream_count = 0;
    status = izp_ffi_stream_range("10^6", 200, 30, stream_out, 0, &stream_count);
    FILE *f = fopen(stream_out, "r");
    int has_bytes = 0;
    if (f)
    {
        has_bytes = (fgetc(f) != EOF);
        fclose(f);
    }
    if (status == IZP_FFI_OK && stream_count > 0 && has_bytes)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "izp_ffi_stream_range", "count=%" PRIu64 " file=%s", stream_count, stream_out);
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "izp_ffi_stream_range", "status=%d count=%" PRIu64 " err=%s", status, stream_count, izp_ffi_last_error());
    }

    print_test_summary(module_name, passed_tests, failed_tests, verbose);
    return (failed_tests == 0) ? 1 : 0;
}
