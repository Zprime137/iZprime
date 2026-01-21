#include <test_api.h>

static int has_factor(mpz_t num, UI64_ARRAY *factors)
{
    for (int i = 0; i < factors->count; i++)
    {
        if (mpz_divisible_ui_p(num, factors->array[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Test function for VX_SEG structure and its related functions.
 *
 * This function tests the VX_SEG implementation by performing a series of operations.
 *
 * @param verbose If non-zero, prints detailed test output.
 * @return int 1 on success, 0 on failure.
 */
int TEST_VX_SEG(int verbose)
{
    char module_name[] = "VX_SEG";
    int passed_tests = 0;
    int failed_tests = 0;
    int current_test_idx = 0;
    int current_test_result = 1;

    // Print module header
    print_module_header(module_name);
    if (verbose)
        print_test_header();

    int vx = VX6;
    IZM *iZm = iZm_init(vx);
    if (iZm == NULL)
    {
        printf("TEST_VX_SEG failed critically at iZm_init. Aborting further tests.\n");
        return 0;
    }

    // * Test vx_init
    current_test_idx++;
    VX_SEG *test_obj = vx_init(iZm, 1, vx, "1000000000", 5);

    if (test_obj == NULL)
    {
        printf("TEST_VX_SEG failed critically at vx_init. Aborting further tests.\n");
        return 0;
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_init", "test_obj initialization successful with vx=%d", vx);
        }
    }

    // * Test vx_det_sieve
    current_test_idx++;

    mpz_t test_num;
    mpz_init(test_num);
    current_test_result = 1;
    // test if some umarked bits has factors < vx
    for (int x = 1; x < 1000; x++)
    {
        if (bitmap_get_bit(test_obj->x5, x))
        {
            // check if iZ(vx * y + x, -1) has a factor smaller than vx
            mpz_add_ui(test_num, test_obj->yvx, x);
            iZ_mpz(test_num, test_num, -1);
            if (has_factor(test_num, iZm->root_primes))
            {
                current_test_result = 0;
                break;
            }
        }

        if (bitmap_get_bit(test_obj->x7, x))
        {
            // check if iZ(vx * y + x, -1) has a factor smaller than vx
            mpz_add_ui(test_num, test_obj->yvx, x);
            iZ_mpz(test_num, test_num, 1);
            if (has_factor(test_num, iZm->root_primes))
            {
                current_test_result = 0;
                break;
            }
        }
    }

    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_det_sieve", "Deterministic sieving seems correct");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_det_sieve", "Deterministic sieving failed. Aborting further tests.");
            vx_free(&test_obj);
            iZm_free(&iZm);
            return 0;
        }
    }

    // * Test vx_full_sieve
    current_test_idx++;
    vx_full_sieve(test_obj, 0);
    current_test_result = 1;
    // test the primality of some unmarked bits
    for (int x = 1; x < 1000; x++)
    {
        if (bitmap_get_bit(test_obj->x5, x))
        {
            // check if iZ(vx * y + x, -1) is prime
            mpz_add_ui(test_num, test_obj->yvx, x);
            iZ_mpz(test_num, test_num, -1);
            if (!mpz_probab_prime_p(test_num, test_obj->mr_rounds))
            {
                current_test_result = 0;
                break;
            }
        }

        if (bitmap_get_bit(test_obj->x7, x))
        {
            // check if iZ(vx * y + x, 1) is prime
            mpz_add_ui(test_num, test_obj->yvx, x);
            iZ_mpz(test_num, test_num, 1);
            if (!mpz_probab_prime_p(test_num, test_obj->mr_rounds))
            {
                current_test_result = 0;
                break;
            }
        }
    }

    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_full_sieve, vx_prob_sieve", "Full sieving seems correct");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_full_sieve, vx_prob_sieve", "Full sieving failed");
        }
    }

    // * Test vx_collect_p_gaps
    current_test_idx++;
    vx_collect_p_gaps(test_obj);
    if (test_obj->p_gaps != NULL)
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_collect_p_gaps", "Prime gaps collected successfully");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_collect_p_gaps", "Failed to collect p_gaps");
        }
    }

    // * Test: vx_free
    current_test_idx++;

    vx_free(&test_obj);
    // if no memory errors detected, consider test passed
    if (test_obj == NULL)
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_free", "VX_SEG memory freed successfully");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_free", "VX_SEG memory freeing failed");
        }
    }

    // * Test vx_stream_file
    current_test_idx++;
    FILE *stream_file = fopen("./output/test_vx_seg_streamed_primes.txt", "w");
    if (stream_file)
    {
        VX_SEG *vx_s = vx_init(iZm, 1, vx, "1000000000000000", 25);
        vx_stream_file(vx_s, stream_file);
        fclose(stream_file);
        vx_free(&vx_s);
        passed_tests++;

        if (verbose)
        {
            log_test(1, current_test_idx, "vx_stream_file", "Streaming primes to file successful");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_stream_file", "Failed to open file for streaming primes");
        }
    }

    iZm_free(&iZm);

    // * Print test summary
    log_test_summary(module_name, passed_tests, failed_tests, verbose);
    return (failed_tests == 0) ? 1 : 0;
}
