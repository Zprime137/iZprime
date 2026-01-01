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
 * Tests initialization, base value setting, sieving, file I/O, and memory management.
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

    // * Test 1: vx_init
    current_test_idx++;
    int vx = VX4;
    VX_SEG *vx_obj = vx_init(vx, "1000000000", 5);

    if (vx_obj == NULL)
    {
        printf("TEST_VX_SEG failed critically at vx_init. Aborting further tests.\n");
        return 0;
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_init", "IZM initialization successful with vx=%d", vx);
        }
    }

    // * Test 2: vx_det_sieve
    current_test_idx++;
    IZM *iZm = iZm_init(vx, vx);
    if (iZm == NULL)
    {
        printf("TEST_VX_SEG failed critically at iZm_init. Aborting further tests.\n");
        vx_free(&vx_obj);
        return 0;
    }
    vx_det_sieve(iZm, vx_obj);
    if (vx_obj->x5 == NULL && vx_obj->x7 == NULL)
    {
        printf("TEST_VX_SEG failed critically at vx_det_sieve. Aborting further tests.\n");
        iZm_free(&iZm);
        vx_free(&vx_obj);
        return 0;
    }

    mpz_t test_num;
    mpz_init(test_num);
    current_test_result = 1;
    // test if some umarked bits has factors < vx
    for (int x = 1; x < 1000; x++)
    {
        if (bitmap_get_bit(vx_obj->x5, x))
        {
            // check if iZ(vx * y + x, -1) has a factor smaller than vx
            mpz_add_ui(test_num, vx_obj->yvx, x);
            iZ_mpz(test_num, test_num, -1);
            if (has_factor(test_num, iZm->root_primes))
            {
                current_test_result = 0;
                break;
            }
        }

        if (bitmap_get_bit(vx_obj->x7, x))
        {
            // check if iZ(vx * y + x, -1) has a factor smaller than vx
            mpz_add_ui(test_num, vx_obj->yvx, x);
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
            vx_free(&vx_obj);
            iZm_free(&iZm);
            return 0;
        }
    }

    // * Test 3: vx_full_sieve
    current_test_idx++;
    vx_full_sieve(iZm, vx_obj, 0);
    current_test_result = 1;
    // test the primality of some unmarked bits
    for (int x = 1; x < 1000; x++)
    {
        if (bitmap_get_bit(vx_obj->x5, x))
        {
            // check if iZ(vx * y + x, -1) is prime
            mpz_add_ui(test_num, vx_obj->yvx, x);
            iZ_mpz(test_num, test_num, -1);
            if (!mpz_probab_prime_p(test_num, vx_obj->mr_rounds))
            {
                current_test_result = 0;
                break;
            }
        }

        if (bitmap_get_bit(vx_obj->x7, x))
        {
            // check if iZ(vx * y + x, 1) is prime
            mpz_add_ui(test_num, vx_obj->yvx, x);
            iZ_mpz(test_num, test_num, 1);
            if (!mpz_probab_prime_p(test_num, vx_obj->mr_rounds))
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
            log_test(1, current_test_idx, "vx_full_sieve", "Full sieving seems correct");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_full_sieve", "Full sieving failed");
        }
    }

    // * Test 4: vx_collect_p_gaps
    current_test_idx++;
    vx_collect_p_gaps(vx_obj);
    if (vx_obj->p_gaps != NULL)
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

    // * Test 5: vx_nth_p
    current_test_idx++;
    mpz_t expected_prime;
    mpz_init(expected_prime);
    int n = 100; // get the nth prime in the segment

    if (vx_nth_p(test_num, vx_obj, n))
    {
        // verify by direct calculation
        int gap_sum = 0;
        int s = n > 0 ? 0 : vx_obj->p_gaps->count - n;
        int e = n > 0 ? n : vx_obj->p_gaps->count;

        for (int i = s; i < e; i++)
        {
            gap_sum += vx_obj->p_gaps->array[i];
        }

        iZ_mpz(expected_prime, vx_obj->yvx, 1); // iZ(vx * y, 1)
        if (n > 0)
        {
            mpz_add_ui(expected_prime, expected_prime, gap_sum);
        }
        else
        {
            mpz_add_ui(expected_prime, expected_prime, 6 * vx_obj->vx);
            mpz_sub_ui(expected_prime, expected_prime, gap_sum);
        }

        // confirm equality
        if (mpz_cmp(test_num, expected_prime) == 0)
        {
            passed_tests++;
            if (verbose)
            {
                log_test(1, current_test_idx, "vx_nth_p", "%dth prime retrieved successfully", n);
            }
        }
        else
        {
            failed_tests++;
            if (verbose)
            {
                log_test(0, current_test_idx, "vx_nth_p", "nth prime value incorrect");
            }
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_nth_p", "Failed to retrieve nth prime");
        }
    }
    mpz_clear(expected_prime);

    // * Test 6: vx_write_file
    current_test_idx++;
    const char *file_path = "./output/test_vx_seg";
    if (vx_write_file(vx_obj, (char *)file_path))
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_write_file", "Writing VX_SEG to file successful");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_write_file", "Failed to write VX_SEG to file");
        }
    }

    // * Test 7: vx_read_file
    current_test_idx++;
    VX_SEG *vx_obj_read = vx_read_file((char *)file_path);
    if (vx_obj_read != NULL &&
        vx_obj_read->vx == vx_obj->vx &&
        mpz_cmp(vx_obj_read->y, vx_obj->y) == 0 &&
        vx_obj_read->p_count == vx_obj->p_count)
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "vx_read_file", "Reading VX_SEG from file successful");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            log_test(0, current_test_idx, "vx_read_file", "Reading VX_SEG from file failed");
        }
    }
    vx_free(&vx_obj_read);

    mpz_clear(test_num);
    iZm_free(&iZm);
    vx_free(&vx_obj);

    // * Print test summary
    log_test_summary(module_name, passed_tests, failed_tests, verbose);
    return (failed_tests == 0 && current_test_result) ? 1 : 0;
}
