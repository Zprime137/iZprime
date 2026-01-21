#include <test_api.h>

int TEST_IZM(int verbose)
{
    char module_name[] = "IZM";
    int passed_tests = 0;
    int failed_tests = 0;
    int current_test_idx = 0;
    int current_test_result = 1;

    print_module_header(module_name);

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
        print_test_header();
        log_test(1, current_test_idx, "iZm_init", "Initialization with vx=%d successful", vx);
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
                    log_test(0, current_test_idx, "iZm_construct_vx_base", "iZm5 base construction incorrect at x=%d", x);
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
                    log_test(0, current_test_idx, "iZm_construct_vx_base", "iZm7 base construction incorrect at x=%d", x);
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
            log_test(1, current_test_idx, "iZm_construct_vx_base", "Base construction for iZm5 and iZm7 correct");
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
            log_test(0, current_test_idx, "iZm_free", "Pointer not nullified after free");
        }
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "iZm_free", "Memory freed and pointer nullified successfully");
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

        uint64_t xp = iZm_solve_for_xp(m_id, p, vx, test_y);
        uint64_t z = iZ(xp + vx * test_y, m_id);
        // z should be composite (divisible by p)
        if (z % p != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                log_test(0, current_test_idx, "iZm_solve_for_x", "Composite targeting failed for p=%lld", p);
            }
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "iZm_solve_for_x", "Composite targeting correct for all test primes");
        }
    }

    // * Test 5: iZm_solve_for_x_mpz
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
        uint64_t xp = iZm_solve_for_xp_mpz(m_id, p, vx, mpz_y);
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
                log_test(0, current_test_idx, "iZm_solve_for_x_mpz", "Composite targeting failed for p=%lld", p);
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
            log_test(1, current_test_idx, "iZm_solve_for_x_mpz", "Composite targeting correct for all test primes");
        }
    }

    // * Test 6: iZm_solve_for_yp
    current_test_idx++;
    current_test_result = 1; // reset
    uint64_t test_x = 17;    // some arbitrary x where gcd(iZ(test_x, m_id), vx) == 1

    for (int i = 0; i < num_tests; i++)
    {
        uint64_t p = test_primes[i];
        uint64_t yp = iZm_solve_for_yp(m_id, p, vx, test_x);
        uint64_t z = iZ(test_x + vx * yp, m_id);
        if (z % p != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                log_test(0, current_test_idx, "iZm_solve_for_yp", "Composite targeting failed for p=%lld", p);
            }
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            log_test(1, current_test_idx, "iZm_solve_for_yp", "Composite targeting correct for all test primes");
        }
    }

    log_test_summary(module_name, passed_tests, failed_tests, verbose);

    return (failed_tests == 0) ? 1 : 0;
}
