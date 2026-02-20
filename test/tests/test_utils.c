#include <test_api.h>

int TEST_UTILS(int verbose)
{
    char module_name[] = "UTILS";
    int passed_tests = 0;
    int failed_tests = 0;
    int current_test_idx = 0;

    print_test_module_header(module_name);
    if (verbose)
        print_test_table_header();

    // Test 1: power notation parser
    current_test_idx++;
    uint64_t u64_value = 0;
    if (parse_numeric_expr_u64("10^6", &u64_value) && u64_value == 1000000ULL)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_numeric_expr_u64", "10^6 -> 1000000");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_numeric_expr_u64", "Failed to parse 10^6");
    }

    // Test 2: scientific shorthand parser
    current_test_idx++;
    if (parse_numeric_expr_u64("1e6", &u64_value) && u64_value == 1000000ULL)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_numeric_expr_u64", "1e6 -> 1000000");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_numeric_expr_u64", "Failed to parse 1e6");
    }

    // Test 3: grouped decimal parser
    current_test_idx++;
    if (parse_numeric_expr_u64("1,000,000", &u64_value) && u64_value == 1000000ULL)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_numeric_expr_u64", "1,000,000 -> 1000000");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_numeric_expr_u64", "Failed to parse grouped decimal");
    }

    // Test 4: additive expression parser
    current_test_idx++;
    if (parse_numeric_expr_u64("10e3 + 5", &u64_value) && u64_value == 10005ULL)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_numeric_expr_u64", "10e3 + 5 -> 10005");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_numeric_expr_u64", "Failed additive expression parse");
    }

    // Test 5: very large expression parser (mpz)
    current_test_idx++;
    mpz_t parsed_mpz, expected_mpz;
    mpz_inits(parsed_mpz, expected_mpz, NULL);
    int ok_mpz = parse_numeric_expr_mpz(parsed_mpz, "10e100 + 10e9");
    mpz_ui_pow_ui(expected_mpz, 10, 101);
    mpz_add_ui(expected_mpz, expected_mpz, 10000000000ULL);
    if (ok_mpz && mpz_cmp(parsed_mpz, expected_mpz) == 0)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_numeric_expr_mpz", "Large expression parsed correctly");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_numeric_expr_mpz", "Failed large expression parse");
    }
    mpz_clears(parsed_mpz, expected_mpz, NULL);

    // Test 6: inclusive range parser with grouped decimals
    current_test_idx++;
    mpz_t lower, upper;
    mpz_inits(lower, upper, NULL);
    int ok_range = parse_inclusive_range_mpz("[1,000,000, 1,000,100]", lower, upper);
    if (ok_range && mpz_cmp_ui(lower, 1000000ULL) == 0 && mpz_cmp_ui(upper, 1000100ULL) == 0)
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_inclusive_range_mpz", "Grouped range parsed correctly");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_inclusive_range_mpz", "Failed grouped range parse");
    }
    mpz_clears(lower, upper, NULL);

    // Test 7: invalid grouped decimal should fail
    current_test_idx++;
    if (!parse_numeric_expr_u64("1,00,000", &u64_value))
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_numeric_expr_u64", "Rejects invalid grouped decimal");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_numeric_expr_u64", "Accepted invalid grouped decimal");
    }

    // Test 8: invalid range expression should fail
    current_test_idx++;
    mpz_inits(lower, upper, NULL);
    if (!parse_inclusive_range_mpz("range[10^6]", lower, upper))
    {
        passed_tests++;
        if (verbose)
            print_test_module_result(1, current_test_idx, "parse_inclusive_range_mpz", "Rejects malformed range expression");
    }
    else
    {
        failed_tests++;
        if (verbose)
            print_test_module_result(0, current_test_idx, "parse_inclusive_range_mpz", "Accepted malformed range expression");
    }
    mpz_clears(lower, upper, NULL);

    print_test_summary(module_name, passed_tests, failed_tests, verbose);
    return (failed_tests == 0) ? 1 : 0;
}
