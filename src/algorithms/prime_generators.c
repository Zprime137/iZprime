/**
 * @file prime_gen.c
 * @brief This file contains the implementation of various primes generation functions.
 *
 * @description:
 * This file implements the following prime generation functions:
 * @b vy_random_prime: A function to generate a random prime number of a given bit-size and supports multi-core processing.
 * @b vx_random_prime: A function to generate a random prime number using VX-based search with multi-core support.
 * @b iZ_next_prime: A function to find the next/previous prime number after a given base.
 * @b iZ_random_next_prime: A function to generate a random prime number using the iZ-next-prime method.
 * @b gmp_random_next_prime: A function to generate a random prime number using GMP's mpz_nextprime function.
 *
 */

#include <iZ_api.h>

// Helper to generate a random m_id (1 or -1)
static int random_m_id(void)
{
    return (rand() % 2) ? 1 : -1;
}

/**
 * @brief vertical search routine for generating a random prime.
 *
 * @description: This function searches for a prime number using the given parameters.
 * It combines the iZ-Matrix filtering techniques with the Miller-Rabin primality test.
 * The search is performed by finding a suitable x value that does not correspond to
 * a composite of a prime that divides vx. Then, it iterates over y value in the
 * equation p = iZ(x + vx * y, m_id) until a prime is found.
 *
 * @param p The prime number found in the search.
 * @param m_id The identifier (1 or -1) for the iZ matrix.
 * @param vx The horizontal vector of the iZ matrix.
 * @return 1 if a prime is found, 0 otherwise.
 */
static int vy_search_prime(mpz_t p, int m_id, mpz_t vx)
{
    assert(p && vx && "Input parameters cannot be NULL");

    int found = 0; // flag to indicate if a prime was found

    // set m_id randomly if not provided
    if (m_id != -1 && m_id != 1)
        m_id = random_m_id();

    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state); // seed random state

    mpz_t z, g;
    mpz_init(z);
    mpz_init(g);

    // set random x value in the range of vx
    mpz_urandomm(z, state, vx);
    // compute z = 6 * z + i
    iZ_mpz(z, z, m_id);

    // search for x value such that gcd(iZ(x, m_id), z) = 1
    for (;;)
    {
        // increment z by 6 to advance x by 1
        mpz_add_ui(z, z, 6);

        // Compute g = gcd(vx, z)
        mpz_gcd(g, vx, z);

        // break if g = 1,
        // implying current x value can yield primes of the form iZ(x + vx * y, p_id)
        if (mpz_cmp_ui(g, 1) == 0)
            break;
    }

    // set g = 6 * vx to use as a step
    mpz_mul_ui(g, vx, 6);
    // add z by rand * g
    int rand_steps = rand() % 100;
    mpz_addmul_ui(z, g, rand_steps);

    while (!found)
    {
        // increment z by 6 * vx to advance y by 1
        mpz_add(z, z, g);

        // check if z is prime
        found = mpz_probab_prime_p(z, MR_ROUNDS);

        // if z is prime, set p = z
        if (found)
            mpz_set(p, z);
    }

    // cleanup
    gmp_randclear(state);
    mpz_clears(z, g, NULL);

    return found;
}

/**
 * @brief Generates a random prime candidate using the vy_search_prime routine.
 *
 * @description: This function generates a random prime of a given bit size using the vy_search_prime routine.
 * It initializes the random base and sets up the search parameters. The function also allows
 * for parallel processing of the search using multiple child processes. The generated prime
 * is stored in the provided mpz_t p variable.
 *
 * @param p The mpz_t variable to store the generated prime number.
 * @param bit_size The target bit size of the prime.
 * @param cores_num The number of cores to use for parallel processing.
 * @return 1 if a prime is found, 0 otherwise.
 */
int vy_random_prime(mpz_t p, int bit_size, int cores_num)
{
    int found = 0;                // flag to indicate if a prime was found
    bit_size = MAX(bit_size, 10); // Set minimum bit size

    // 1. Compute max vx for the given bit-size
    mpz_t vx;
    mpz_init(vx);
    iZm_compute_max_vx(vx, bit_size);

    // 2. If < 2 cores, run the search in-process
    if (cores_num < 2)
    {
        found = vy_search_prime(p, 0, vx);
        mpz_clear(vx);
        return found;
    }

    // 3. Else, fork multiple processes to search for a prime
    // Create a pipe for inter-process communication.
    int fd[2];
    if (pipe(fd) == -1)
    {
        log_error("Failed to create pipe in vy_random_prime. Falling back to in-process search.");
        found = vy_search_prime(p, 0, vx);
        mpz_clear(vx);
        return found;
    }

    pid_t pids[cores_num];
    int forks_created = 0;

    // Fork child processes.
    for (int i = 0; i < cores_num; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            log_error("Failed to fork process %d in vy_random_prime", i);
            continue; // Skip this fork failure and continue creating other forks
        }
        else if (pid == 0)
        {
            // Child process: close the read-end.
            close(fd[0]);

            mpz_t local_p; // local prime candidate
            mpz_init(local_p);

            // Search for a candidate prime
            found = vy_search_prime(local_p, 0, vx);

            // If a candidate is found, send it via the pipe.
            if (found)
            {
                char *p_str = mpz_get_str(NULL, 10, local_p);
                if (p_str)
                {
                    write(fd[1], p_str, strlen(p_str) + 1);
                    free(p_str);
                }
            }
            mpz_clear(local_p);

            close(fd[1]); // Close the write-end.
            exit(0);
        }
        else
        {
            // Parent process saves child's PID.
            pids[i] = pid;
            forks_created++;
        }
    }
    mpz_clear(vx);

    // 4. Parent reads first result from the pipe.
    close(fd[1]); // Close the write-end.

    if (forks_created == 0)
    {
        log_error("No child processes were created in vy_random_prime, falling back to in-process search");
        close(fd[0]);
        found = vy_search_prime(p, 0, vx);
        return found;
    }

    // Allocate a sufficiently large buffer to hold the prime string + null terminator
    // bit_size * 0.302 is approx digits, add extra padding for safety
    size_t buf_size = (size_t)(bit_size * 0.4);
    char *buf = malloc(buf_size);
    if (!buf)
    {
        log_error("Failed to allocate buffer in vy_random_prime");
        found = 0;
    }
    else
    {
        ssize_t n = read(fd[0], buf, buf_size - 1);
        if (n == -1)
        {
            log_error("Failed to read from pipe in vx_random_prime");
            found = 0;
        }
        else if (n > 0)
        {
            buf[n] = '\0'; // Ensure null-termination
            if (mpz_set_str(p, buf, 10) == 0)
            {
                found = 1;
            }
            else
            {
                log_error("Failed to set prime from buffer in vx_random_prime");
                found = 0;
            }
        }
        free(buf);
    }
    close(fd[0]); // Close the read-end of the pipe.

    // 5. Terminate all child processes
    for (int i = 0; i < forks_created; i++)
    {
        kill(pids[i], SIGTERM);    // Terminate child process
        waitpid(pids[i], NULL, 0); // Wait for child process to terminate
    }

    return found;
}

/**
 * @brief horizontal search routine for generating a random prime.
 *
 * @description: This function searches for a prime number using the given parameters.
 * It combines the iZ-Matrix filtering techniques with the Miller-Rabin primality test.
 * The search is performed by finding suitable x values that do not correspond to
 * composites of primes that divide vx. Then, it iterates over y values in the
 * equation p = iZ(x + vx * y) until a prime is found.
 *
 * @param p The prime number found in the search.
 * @param m_id The identifier (1 or -1) for the iZ matrix.
 * @param vx The horizontal vector of the iZ matrix.
 * @param bit_size The target bit size of the prime.
 * @return 1 if a prime is found, 0 otherwise.
 */
static int vx_search_prime(mpz_t p, int m_id, int vx, int bit_size)
{
    // assert p is not NULL
    assert(p && "p cannot be NULL in vx_search_prime.");

    bit_size = MAX(bit_size, 10);

    // set m_id randomly if not provided
    if (m_id != -1 && m_id != 1)
        m_id = random_m_id();

    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state);

    UI64_ARRAY *root_primes = SiZm(vx);
    if (!root_primes)
    {
        log_error("Failed to initialize root primes in vx_search_prime");
        gmp_randclear(state);
        return 0;
    }

    mpz_t z, x_z, y, yvx;
    mpz_init(z);
    mpz_init(x_z);
    mpz_init(y);
    mpz_init(yvx);

    mpz_urandomb(y, state, bit_size);
    mpz_fdiv_q_ui(y, y, 6 * vx);
    mpz_mul_ui(yvx, y, vx); // compute yvx = vx * y

    int found = 0;
    while (!found)
    {
        BITMAP *bitmap = bitmap_init(vx + 10, 1);

        // sieve the bitmap with root primes skipping 2 and 3
        for (int i = 2; i < root_primes->count; i++)
        {
            uint64_t q = root_primes->array[i];
            // mark composites of q in the bitmap
            bitmap_clear_steps(bitmap, q, iZm_solve_for_xp_mpz(m_id, q, vx, y), vx);
        }

        int random_x = rand() % (vx / 2); // random x < vx/2
        for (int x = random_x; x < vx; x++)
        {
            // check if z is prime candidate in bitmap
            if (bitmap_get_bit(bitmap, x))
            {
                // compute x_z = vx * y + x
                mpz_add_ui(x_z, yvx, x);
                iZ_mpz(z, x_z, m_id);
                // check if z is prime
                found = mpz_probab_prime_p(z, MR_ROUNDS);

                // if z is prime, set p = z
                if (found)
                {
                    mpz_set(p, z);
                    break;
                }
            }
        }

        // cleanup
        bitmap_free(&bitmap);

        // increment y by 1 for next vx segment
        mpz_add_ui(y, y, 1);
        mpz_add_ui(yvx, yvx, vx);
    }

    ui64_free(&root_primes);
    mpz_clears(z, x_z, y, yvx, NULL);
    gmp_randclear(state);

    return found;
}

/**
 * @brief Generates a random prime candidate using the vx_search_prime routine.
 *
 * @description: This function generates a random prime of a given bit size using the vx_search_prime routine.
 * It initializes the random base and sets up the search parameters. The function also allows
 * for parallel processing of the search using multiple child processes. The generated prime
 * is stored in the provided mpz_t p variable.
 *
 * @param p The mpz_t variable to store the generated prime number.
 * @param bit_size The target bit size of the prime.
 * @param cores_num The number of cores to use for parallel processing.
 * @return 1 if a prime is found, 0 otherwise.
 */
int vx_random_prime(mpz_t p, int bit_size, int cores_num)
{
    int found = 0;
    bit_size = MAX(bit_size, 10);
    int vx = bit_size <= 2048 ? VX5 : VX6;

    // 2. If < 2 cores, run the search in-process
    if (cores_num < 2)
    {
        found = vx_search_prime(p, 0, vx, bit_size);
        return found;
    }

    // 3. Multi-core: fork processes
    int fd[2];
    if (pipe(fd) == -1)
    {
        log_error("Failed to create pipe in vx_random_prime. Falling back to in-process search.");
        found = vx_search_prime(p, 0, vx, bit_size);
        return found;
    }

    pid_t pids[cores_num];
    int forks_created = 0;

    for (int i = 0; i < cores_num; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            log_error("Failed to fork process %d in vx_random_prime", i);
            continue;
        }
        else if (pid == 0)
        {
            // CHILD PROCESS: Initialize everything fresh
            close(fd[0]);

            // Search for prime with child's own objects
            mpz_t local_p;
            mpz_init(local_p);
            int child_found = vx_search_prime(local_p, 0, vx, bit_size);

            if (child_found)
            {
                char *p_str = mpz_get_str(NULL, 10, local_p);
                if (p_str != NULL)
                {
                    write(fd[1], p_str, strlen(p_str) + 1);
                    free(p_str);
                }
            }

            // Cleanup child resources
            mpz_clear(local_p);
            close(fd[1]);
            exit(0);
        }
        else
        {
            pids[i] = pid;
            forks_created++;
        }
    }

    // 4. Parent process reads result
    close(fd[1]);

    if (forks_created == 0)
    {
        log_error("No child processes were created in vx_random_prime, falling back to in-process search");
        close(fd[0]);
        // fallback to in-process search
        found = vx_search_prime(p, 0, vx, bit_size);
        return found;
    }

    // Allocate a sufficiently large buffer to hold the prime string + null terminator
    size_t buf_size = (size_t)(bit_size * 0.4);
    char *buf = malloc(buf_size);
    if (!buf)
    {
        log_error("Failed to allocate buffer in vx_random_prime");
        found = 0;
    }
    else
    {
        ssize_t n = read(fd[0], buf, buf_size - 1);
        if (n > 0)
        {
            buf[n] = '\0'; // Ensure null-termination
            if (mpz_set_str(p, buf, 10) == 0)
            {
                found = 1;
            }
            else
            {
                log_error("Failed to set prime from buffer in vx_random_prime");
                found = 0;
            }
        }
        free(buf);
    }

    close(fd[0]); // Close the read-end of the pipe.

    // Terminate children
    for (int i = 0; i < forks_created; i++)
    {
        kill(pids[i], SIGTERM);
        waitpid(pids[i], NULL, 0);
    }

    return found;
}

/**
 * @brief Find the next prime number after a given base.
 *
 * @description: This function searches for the next/previous prime number after a given base using the iZ framework.
 *
 * @param p The mpz_t variable to store the found prime number.
 * @param base The base number to start the search from.
 * @param forward If true, search for the next prime; if false, search for the previous prime.
 * @return 1 if a prime is found, 0 otherwise.
 */
int iZ_next_prime(mpz_t p, mpz_t base, int forward)
{
    // 1. Initialization
    int found = 0; // flag to indicate if a prime was found

    // tmp variable to hold the value until a prime is found
    mpz_t z;
    mpz_init_set(z, base); // set tmp = base

    // a. Edge cases:
    // if forward and base is iZ-, check next iZ+
    if (mpz_fdiv_ui(z, 6) == 5 && forward)
    {
        mpz_add_ui(z, z, 2); // increment tmp by 2
        if (mpz_probab_prime_p(z, MR_ROUNDS))
        {
            mpz_set(p, z); // set p = tmp + 2
            mpz_clear(z);
            return 1;
        }
    }
    // if backward and base is iZ+, check previous iZ-
    else if (mpz_fdiv_ui(z, 6) == 1 && !forward)
    {
        mpz_sub_ui(z, z, 2); // decrement tmp by 2
        if (mpz_probab_prime_p(z, MR_ROUNDS))
        {
            mpz_set(p, z); // set p = tmp - 2
            mpz_clear(z);
            return 1;
        }
    }

    int vx = (mpz_sizeinbase(base, 2) > 2048) ? VX6 : VX5;
    IZM *iZm = iZm_init(vx);
    if (!iZm)
    {
        log_error("iZm initialization failed in iZ_next_prime.");
        mpz_clear(z);
        return 0;
    }

    // c. Initialize and set y and yvx
    mpz_t y, yvx, x_p;
    mpz_init(y);
    mpz_init(yvx);
    mpz_init(x_p);

    mpz_div_ui(y, base, 6 * vx); // compute y = base / 6 * vx
    mpz_mul_ui(yvx, y, vx);      // compute yvx = y * vx
    mpz_div_ui(x_p, z, 6);       // compute x_p = tmp / 6

    // 2. Iterate over the x5 and x7 bitmaps to find a prime
    // set start_x = x_p % vx +/- 1
    int step = forward ? 1 : -1;
    int start_x = mpz_fdiv_ui(x_p, vx) + step;
    int end_x = forward ? vx : 1;

    int i = 0; // segment counter

    while (!found)
    {
        if (forward)
        {
            if (i > 0)
                start_x = 1; // start from the beginning of the bitmap

            for (int x = start_x; x <= end_x; x++)
            {
                // check if x5[x] is set
                if (bitmap_get_bit(iZm->base_x5, x))
                {
                    mpz_add_ui(x_p, yvx, x); // set x_p = yvx + x
                    iZ_mpz(z, x_p, -1);      // compute p = iZ(x_p, -1)
                    // check if tmp is prime
                    found = mpz_probab_prime_p(z, MR_ROUNDS);

                    if (found)
                        break;
                }

                // check if x7[x] is set
                if (bitmap_get_bit(iZm->base_x7, x))
                {
                    mpz_add_ui(x_p, yvx, x); // set x_p = yvx + x
                    iZ_mpz(z, x_p, 1);       // compute tmp = iZ(x_p, 1)
                    // check if tmp is prime
                    found = mpz_probab_prime_p(z, MR_ROUNDS);

                    if (found)
                        break;
                }
            }

            mpz_add_ui(yvx, yvx, vx); // increment yvx by vx for next segment
        }
        else // backward search
        {
            if (i > 0)
                start_x = vx; // start from the end of the bitmaps

            // check iZ+ first if backward
            for (int x = start_x; x >= end_x; x--)
            {
                // check if x7[x] is set
                if (bitmap_get_bit(iZm->base_x7, x))
                {
                    mpz_add_ui(x_p, yvx, x); // set x_p = yvx + x
                    iZ_mpz(z, x_p, 1);       // compute tmp = iZ(x_p, 1)
                    // check if tmp is prime
                    found = mpz_probab_prime_p(z, MR_ROUNDS);

                    if (found)
                        break;
                }

                // check iZ-
                if (bitmap_get_bit(iZm->base_x5, x))
                {
                    mpz_add_ui(x_p, yvx, x); // set x_p = yvx + x
                    iZ_mpz(z, x_p, -1);      // compute p = iZ(x_p, -1)
                    // check if tmp is prime
                    found = mpz_probab_prime_p(z, MR_ROUNDS);

                    if (found)
                        break;
                }
            }

            mpz_sub_ui(yvx, yvx, vx); // decrement yvx by vx for next segment
        }

        i++; // increment segment counter
    }

    // 3. Set the found prime
    if (found)
        mpz_set(p, z); // set p = tmp
    else
        log_debug("No prime found :/");

    // cleanup
    iZm_free(&iZm);
    mpz_clears(y, yvx, x_p, z, NULL);

    return found;
}

/**
 * @brief A wrapper function to generate a random prime number using
 * the iZ_next_prime function.
 *
 * @param p The mpz_t variable to store the generated prime number.
 * @param bit_size The bit size of the prime number to be generated.
 */
int iZ_random_next_prime(mpz_t p, int bit_size)
{
    // Check if the bit size is within the valid range
    bit_size = MAX(bit_size, 10);

    // Set the initial random number within the magnitude range
    mpz_t base;
    mpz_init(base);

    // initialize the random state
    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state); // seed random state

    // Generate a random number in the given range
    mpz_urandomb(base, state, bit_size);

    // Find the next prime number after the random base
    int found = iZ_next_prime(p, base, 1);

    // cleanup
    mpz_clear(base);
    gmp_randclear(state);

    return found;
}

/**
 * @brief A wrapper function to generate a random prime number using
 * the GMP's mpz_nextprime function.
 *
 * @param p The mpz_t variable to store the generated prime number.
 * @param bit_size The bit size of the prime number to be generated.
 */
int gmp_random_next_prime(mpz_t p, int bit_size)
{
    bit_size = MAX(bit_size, 10);

    // Set the initial random number within the magnitude range
    mpz_t base;
    mpz_init(base);

    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_seed_randstate(state); // seed random state

    // Generate a random number in the given range
    mpz_urandomb(base, state, bit_size);

    // Find the next prime number after the random base
    mpz_nextprime(p, base);

    mpz_clear(base);
    gmp_randclear(state);

    return 1;
}
