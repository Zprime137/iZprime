#include <test_api.h>

int TEST_IZM(int verbose)
{
    char module_name[] = "IZM structure and functions";
    int passed_tests = 0;
    int failed_tests = 0;
    int current_test_idx = 0;
    int current_test_result = 1;

    print_test_module_header(module_name);

    // * Test 1: iZm_init
    current_test_idx++;
    int vx = VX4;
    IZM *iZm = iZm_init(vx);
    if (iZm == NULL || iZm->base_x5 == NULL || iZm->base_x7 == NULL || iZm->root_primes == NULL)
    {
        printf("[FATAL] TEST_IZM failed critically at iZm_init. Aborting further tests.\n");
        return 0;
    }

    passed_tests++;
    if (verbose)
    {
        print_test_table_header();
        print_test_module_result(1, current_test_idx, "iZm_init", "Initialization with vx=%d successful", vx);
    }

    // * Test 2: iZm_construct_vx_base
    current_test_idx++;
    // confirm base construction in iZm_init using iZm_construct_vx_base is correct
    for (int x = 1; x < vx; x++)
    {
        if (bitmap_get_bit(iZm->base_x5, x))
        {
            uint64_t z = iZ(x, -1);
            if (gcd(vx, z) != 1)
            {
                failed_tests++;
                current_test_result = 0;
                if (verbose)
                {
                    print_test_module_result(0, current_test_idx, "iZm_construct_vx_base", "iZm5 base construction incorrect at x=%d", x);
                }
                break;
            }
        }
        if (bitmap_get_bit(iZm->base_x7, x))
        {
            uint64_t z = iZ(x, 1);
            if (gcd(vx, z) != 1)
            {
                failed_tests++;
                current_test_result = 0;
                if (verbose)
                {
                    print_test_module_result(0, current_test_idx, "iZm_construct_vx_base", "iZm7 base construction incorrect at x=%d", x);
                }
                break;
            }
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "iZm_construct_vx_base", "Base construction for iZm5 and iZm7 correct");
        }
    }

    // * Test 3: iZm_free
    current_test_idx++;
    iZm_free(&iZm);
    if (iZm != NULL)
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "iZm_free", "Pointer not nullified after free");
        }
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "iZm_free", "Memory freed and pointer nullified successfully");
        }
    }

    // * Test 4: iZm_solve_for_x
    current_test_idx++;
    current_test_result = 1; // reset
    // some test primes to verify composite targeting
    uint64_t test_primes[] = {29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79};
    int num_tests = sizeof(test_primes) / sizeof(test_primes[0]);
    uint64_t test_y = 10; // some arbitrary y
    int m_id = -1;        // test for iZm5

    for (int i = 0; i < num_tests; i++)
    {
        uint64_t p = test_primes[i];

        uint64_t xp = iZm_solve_for_x0(m_id, p, vx, test_y);
        uint64_t z = iZ(xp + vx * test_y, m_id);
        // z should be composite (divisible by p)
        if (z % p != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "iZm_solve_for_x0", "Composite targeting failed for p=%lld", p);
            }
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "iZm_solve_for_x0", "Composite targeting correct for all test primes");
        }
    }

    // * Test 5: iZm_solve_for_x0_mpz
    current_test_idx++;
    current_test_result = 1; // reset
    mpz_t mpz_z, mpz_y, mpz_x, mpz_vxy;
    mpz_init_set_ui(mpz_y, 1000000000); // some arbitrary big y
    mpz_init(mpz_z);
    mpz_init(mpz_x);
    mpz_init(mpz_vxy);
    mpz_mul_ui(mpz_vxy, mpz_y, vx);

    for (int i = 0; i < num_tests; i++)
    {
        uint64_t p = test_primes[i];
        uint64_t xp = iZm_solve_for_x0_mpz(m_id, p, vx, mpz_y);
        mpz_set_ui(mpz_x, vx);
        mpz_add_ui(mpz_x, mpz_vxy, xp);
        iZ_mpz(mpz_z, mpz_x, m_id);
        int z_mod_p = mpz_mod_ui(mpz_z, mpz_z, p);
        // z should be composite (divisible by p)
        if (z_mod_p != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "iZm_solve_for_x0_mpz", "Composite targeting failed for p=%lld", p);
            }
            break;
        }
    }
    mpz_clears(mpz_y, mpz_z, mpz_x, mpz_vxy, NULL);
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "iZm_solve_for_x0_mpz", "Composite targeting correct for all test primes");
        }
    }

    // * Test 6: iZm_solve_for_y0
    current_test_idx++;
    current_test_result = 1; // reset
    uint64_t test_x = 17;    // some arbitrary x where gcd(iZ(test_x, m_id), vx) == 1

    for (int i = 0; i < num_tests; i++)
    {
        uint64_t p = test_primes[i];
        uint64_t yp = iZm_solve_for_y0(m_id, p, vx, test_x);
        uint64_t z = iZ(test_x + vx * yp, m_id);
        if (z % p != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "iZm_solve_for_y0", "Composite targeting failed for p=%lld", p);
            }
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "iZm_solve_for_y0", "Composite targeting correct for all test primes");
        }
    }

    print_test_summary(module_name, passed_tests, failed_tests, verbose);

    return (failed_tests == 0) ? 1 : 0;
}

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
    print_test_module_header(module_name);
    if (verbose)
        print_test_table_header();

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
            print_test_module_result(1, current_test_idx, "vx_init", "test_obj initialization successful with vx=%d", vx);
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
            print_test_module_result(1, current_test_idx, "vx_det_sieve", "Deterministic sieving seems correct");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "vx_det_sieve", "Deterministic sieving failed. Aborting further tests.");
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
            print_test_module_result(1, current_test_idx, "vx_full_sieve, vx_prob_sieve", "Full sieving seems correct");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "vx_full_sieve, vx_prob_sieve", "Full sieving failed");
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
            print_test_module_result(1, current_test_idx, "vx_collect_p_gaps", "Prime gaps collected successfully");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "vx_collect_p_gaps", "Failed to collect p_gaps");
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
            print_test_module_result(1, current_test_idx, "vx_free", "VX_SEG memory freed successfully");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "vx_free", "VX_SEG memory freeing failed");
        }
    }

    // * Test vx_stream_file
    current_test_idx++;
    FILE *stream_file = fopen("./output/test_vx_seg_streamed_primes.txt", "w");
    if (stream_file)
    {
        VX_SEG *vx_s = vx_init(iZm, 1, vx, "1000000000000000", 25);
        vx_stream(vx_s, stream_file);
        fclose(stream_file);
        vx_free(&vx_s);
        passed_tests++;

        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "vx_stream_file", "Streaming primes to file successful");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "vx_stream_file", "Failed to open file for streaming primes");
        }
    }

    iZm_free(&iZm);

    // * Print test summary
    print_test_summary(module_name, passed_tests, failed_tests, verbose);
    return (failed_tests == 0) ? 1 : 0;
}
