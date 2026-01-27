#include <iZ_api.h>

// =========================================================
// * SiZ Range Variants
// =========================================================

/**
 * @brief Count or stream primes in an arbitrary numeric range using Sieve-iZm.
 *
 * This function counts primes in the interval [Zs, Ze], where Zs is provided
 * as a decimal string and Ze = Zs + range - 1. It maps the numeric bounds
 * into the iZ index space (6x ± 1), partitions that space into segments of
 * length vx, and for each segment invokes the Sieve-iZm machinery via VX
 * segment objects.
 *
 * When an output file path is provided in @p input_range->filepath, the
 * function additionally streams all primes in the requested interval to that
 * file in ascending order, reconstructing them from per-segment prime gaps.
 * Otherwise, it operates in counting mode only.
 *
 * @param input_range Pointer to an INPUT_SIEVE_RANGE structure describing the
 *        start value (as a decimal string), the range length, the optional
 *        output filepath, and Miller–Rabin configuration.
 * @return The total number of primes found in [Zs, Ze] on success, or 0 on
 *         invalid input, allocation failure, or I/O error.
 */
uint64_t SiZ_stream(INPUT_SIEVE_RANGE *input_range)
{
    assert(input_range && input_range->start && "Invalid INPUT_SIEVE_RANGE passed to SiZ_stream.");
    assert(input_range->filepath && "Output filepath is NULL in SiZ_stream.");
    input_range->range = MAX(input_range->range, 100); // enforce minimum range of 100

    FILE *output = fopen(input_range->filepath, "w");
    if (output == NULL)
    {
        log_error("Failed to open output file: %s", input_range->filepath);
        return 0;
    }

    uint64_t total = 0; // output: total prime count

    int vx = VX6; // Use VX6 segment size (1,616,615) for optimal results
    // Miller-Rabin rounds, bounded [5, 50]
    int mr_rounds = MIN(MAX(input_range->mr_rounds, 5), 50);

    // Parse numeric bounds into mpz
    mpz_t Zs, Ze, Xs, Xe, Ys, Ye;
    mpz_init(Zs);
    mpz_init(Ze);
    mpz_init(Xs);
    mpz_init(Xe);
    mpz_init(Ys);
    mpz_init(Ye);

    // read start value Zs
    if (mpz_set_str(Zs, input_range->start, 10) != 0)
    {
        log_error("Invalid numeric string for start.");
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        if (output)
            fclose(output);
        return 0;
    }

    // Ze = Zs + range
    mpz_add_ui(Ze, Zs, input_range->range);

    // Convert bounds to iZ index space:
    mpz_fdiv_q_ui(Xs, Zs, 6); // Xs = Zs/6
    mpz_fdiv_q_ui(Xe, Ze, 6); // Xe = Ze/6

    // Ys = Xs / vx, Ye = Xe / vx
    mpz_fdiv_q_ui(Ys, Xs, vx);
    mpz_fdiv_q_ui(Ye, Xe, vx);

    // start_x = (Xs % vx), end_x = (Xe % vx)
    int start_x = mpz_fdiv_ui(Xs, vx);
    int end_x = mpz_fdiv_ui(Xe, vx);

    // Map modulo 0 to vx to stay in the inclusive x-index range [1..vx]
    if (start_x == 0)
        start_x = 1;
    if (end_x == 0)
        end_x = vx;

    // if Ys = 0, use SiZm for the first segment
    if (mpz_cmp_ui(Ys, 0) == 0)
    {
        uint64_t limit = mpz_cmp_ui(Ye, 0) > 0 ? vx : end_x;
        UI64_ARRAY *primes = SiZm(limit * 6 + 1);
        uint64_t s = mpz_get_ui(Zs);

        for (int i = 0; i < primes->count; i++)
        {
            // only primes > Zs
            if (primes->array[i] > s)
            {
                total++;
                // output prime
                if (output)
                    fprintf(output, "%llu ", primes->array[i]);
            }
        }
        start_x = 1;           // next segment starts at x=1
        mpz_add_ui(Ys, Ys, 1); // increment Ys for the next segment

        ui64_free(&primes);
    }

    // If Ye = 0, we are done
    if (mpz_cmp_ui(Ye, 0) == 0)
    {
        // cleanup
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        if (output)
            fclose(output);
        return total;
    }

    // Initialize iZm structure for vx segments
    IZM *iZm = iZm_init(vx);
    if (!iZm)
    {
        // check logs for errors
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return 0;
    }

    // Process remaining segments for y in (Ys:Ye)
    int y_range = mpz_get_ui(Ye) - mpz_get_ui(Ys);
    mpz_t prime_z;
    mpz_init(prime_z);

    for (int i = 0; i <= y_range; i++)
    {
        int seg_start_x = (i == 0) ? start_x : 1;
        int seg_end_x = (i == y_range) ? end_x : vx;
        VX_SEG *vx_obj = vx_init(iZm, seg_start_x, seg_end_x, mpz_get_str(NULL, 10, Ys), mr_rounds);
        if (!vx_obj)
        {
            // check logs for errors
            iZm_free(&iZm);
            mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, prime_z, NULL);
            if (output)
                fclose(output);
            return 0;
        }

        if (output)
            vx_stream_file(vx_obj, output);
        else
            vx_full_sieve(vx_obj, 0);
        total += vx_obj->p_count; // accumulate prime count

        vx_free(&vx_obj);
        mpz_add_ui(Ys, Ys, 1); // increment Ys for the next segment
    }

    // Cleanup
    iZm_free(&iZm);
    mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, prime_z, NULL);

    if (output)
        fclose(output);

    return total;
}

/**
 * @brief Multi-process prime counting over a numeric range using Sieve-iZm.
 *
 * This function parallelizes prime counting over the interval [Zs, Ze]
 * (interpreted from @p input_range) by partitioning the corresponding iZ
 * index space into VX segments and distributing contiguous blocks of segments
 * across multiple processes. Each child process owns its own cloned IZM
 * instance and VX segments, performs a full Sieve-iZm pass over its assigned
 * segments, and returns a local prime count to the parent via a pipe.
 *
 * The parent process aggregates all child counts, applies boundary
 * corrections for endpoints that do not align exactly with 6x ± 1, and
 * returns the total number of primes in [Zs, Ze]. Any failure to create
 * pipes, fork children, or initialize per-process resources causes the
 * function to log an error and return 0.
 *
 * @param input_range Pointer to an INPUT_SIEVE_RANGE structure describing the
 *        numeric interval and Miller–Rabin configuration.
 * @param cores_num Requested number of worker processes. The actual number
 *        used is clamped to the available CPU cores and the number of VX
 *        segments in the range.
 * @return The total number of primes found in [Zs, Ze] on success, or 0 on
 *         any error or child-process failure.
 */
uint64_t SiZ_count(INPUT_SIEVE_RANGE *input_range, int cores_num)
{
    assert(input_range && input_range->start && input_range->range > 100 &&
           "Invalid INPUT_SIEVE_RANGE passed to SiZ_count.");

    uint64_t total = 0;
    int vx = VX6;             // use VX6 segment size for optimal results
    int vx_interval = 6 * vx; // covers a range of 9,699,690
    cores_num = MIN(cores_num, get_cpu_cores_count());

    uint64_t range = input_range->range;
    int total_segments = (range / vx_interval) + 1;

    // Parse numeric bounds into mpz
    mpz_t Zs, Ze, Xs, Xe, Ys, Ye;
    mpz_init(Zs);
    mpz_init(Ze);
    mpz_init(Xs);
    mpz_init(Xe);
    mpz_init(Ys);
    mpz_init(Ye);

    if (mpz_set_str(Zs, input_range->start, 10) != 0)
    {
        log_error("Invalid numeric string for start.");
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return 0;
    }

    // Convert bounds to iZ index space:
    // Ze = Zs + range - 1
    mpz_add_ui(Ze, Zs, input_range->range - 1);
    mpz_fdiv_q_ui(Xs, Zs, 6); // Xs = Zs/6
    // if Zs % 6 > 1, increment Xs
    if (mpz_fdiv_ui(Zs, 6) > 1)
        mpz_add_ui(Xs, Xs, 1);

    mpz_fdiv_q_ui(Xe, Ze, 6); // Xe = Ze/6

    // Ys = Xs / vx, Ye = Xe / vx
    mpz_fdiv_q_ui(Ys, Xs, vx);
    mpz_fdiv_q_ui(Ye, Xe, vx);

    // start_x = (Xs % vx), end_x = (Xe % vx)
    int start_x = mpz_fdiv_ui(Xs, vx);
    int end_x = mpz_fdiv_ui(Xe, vx);

    // if Ys = 0, use sieve_iZm for the first segment
    if (mpz_cmp_ui(Ys, 0) == 0)
    {
        uint64_t limit = mpz_cmp_ui(Ye, 0) > 0 ? vx : end_x;
        UI64_ARRAY *primes = SiZm(limit * 6 + 1);
        total += primes->count;
        uint64_t s = mpz_get_ui(Zs);

        for (int i = 0; i < primes->count; i++)
        {
            // decrement primes < Zs
            if (primes->array[i] < s)
            {
                total--;
                continue;
            }
        }

        ui64_free(&primes);
        mpz_add_ui(Ys, Ys, 1); // increment Ys for the next segment
        total_segments--;
    }

    IZM *iZm = iZm_init(vx);
    if (!iZm)
    {
        // check logs for errors
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        return 0;
    }

    // Handle edge cases:
    mpz_t prime_z;
    mpz_init(prime_z);
    // if Ys > 0 and Zs % 6 <= 1
    if (mpz_cmp_ui(Ys, 0) > 0 && mpz_fdiv_ui(Zs, 6) <= 1)
    {
        // if iZ(Xs, -1) < Zs and prime, decrement total
        iZ_mpz(prime_z, Xs, -1);
        if (mpz_cmp(prime_z, Zs) < 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }
    // if Ye > 0 and Ze % 6 <= 1
    if (mpz_cmp_ui(Ye, 0) > 0 && mpz_fdiv_ui(Ze, 6) <= 1)
    {
        // if iZ(Xe, 1) > Ze and prime, decrement total
        iZ_mpz(prime_z, Xe, 1);
        if (mpz_cmp(prime_z, Ze) > 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }
    mpz_clear(prime_z);

    // Single-process processing of all segments
    if (cores_num == 1)
    {
        for (int i = 0; i < total_segments; i++)
        {
            int seg_start_x = (i == 0) ? start_x : 1;
            int seg_end_x = (i == total_segments - 1) ? end_x : vx;
            VX_SEG *vx_obj = vx_init(iZm, seg_start_x, seg_end_x, mpz_get_str(NULL, 10, Ys), input_range->mr_rounds);
            if (!vx_obj)
            {
                mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
                iZm_free(&iZm);
                return 0;
            }

            vx_full_sieve(vx_obj, 0);
            total += vx_obj->p_count;

            vx_free(&vx_obj);
            mpz_add_ui(Ys, Ys, 1);
        }
        mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
        iZm_free(&iZm);

        return total;
    }

    // Multi-process processing of remaining segments
    // if total_segments < cores_num, adjust cores_num
    if (total_segments < cores_num)
    {
        cores_num = total_segments;
    }

    int segments_per_core = total_segments / cores_num;
    int remainder_segments = total_segments % cores_num;

    // uint64_t root_limit = mpz_cmp_ui(Ze, INT64_MAX) < 0 ? sqrt(mpz_get_ui(Ze)) + 1 : INT32_MAX;

    int pipe_fds[cores_num][2];
    pid_t pids[cores_num];
    for (int i = 0; i < cores_num; i++)
    {
        pids[i] = -1;
        pipe_fds[i][0] = -1;
        pipe_fds[i][1] = -1;
    }

    for (int core = 0; core < cores_num; core++)
    {
        if (pipe(pipe_fds[core]) == -1)
        {
            log_error("SiZ_count: Failed to create pipe for core %d. Aborting.", core);
            // cleanup and abort on any pipe error
            for (int j = 0; j <= core; j++)
            {
                if (pipe_fds[j][0] != -1)
                    close(pipe_fds[j][0]);
                if (pipe_fds[j][1] != -1)
                    close(pipe_fds[j][1]);
            }
            mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
            iZm_free(&iZm);
            return 0;
        }

        pids[core] = fork();
        if (pids[core] < 0)
        {
            log_error("SiZ_count: Failed to fork process for core %d. Aborting.", core);
            // close this core's pipe and any previous ones, then exit
            for (int j = 0; j <= core; j++)
            {
                if (pipe_fds[j][0] != -1)
                    close(pipe_fds[j][0]);
                if (pipe_fds[j][1] != -1)
                    close(pipe_fds[j][1]);
            }
            mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
            iZm_free(&iZm);
            return 0;
        }

        if (pids[core] == 0)
        {
            // Child process
            // Close unrelated pipes
            for (int j = 0; j < cores_num; j++)
            {
                if (j != core && pipe_fds[j][0] != -1)
                    close(pipe_fds[j][0]);
                if (j != core && pipe_fds[j][1] != -1)
                    close(pipe_fds[j][1]);
            }
            close(pipe_fds[core][0]); // close read end

            // Compute this child's starting Y and number of segments
            int offset = core * segments_per_core + (core < remainder_segments ? core : remainder_segments);
            int local_segments = segments_per_core + (core < remainder_segments ? 1 : 0);

            uint64_t child_total = 0;

            if (local_segments > 0)
            {
                mpz_t local_Ys;
                mpz_init(local_Ys);
                mpz_set(local_Ys, Ys);
                mpz_add_ui(local_Ys, local_Ys, offset);

                // Each child has its own IZM to avoid data races
                IZM *iZm_local = iZm_clone(iZm);
                if (!iZm_local)
                {
                    mpz_clear(local_Ys);
                    close(pipe_fds[core][1]);
                    exit(1);
                }

                for (int i = 0; i < local_segments; i++)
                {
                    int seg_start_x = (core == 0 && i == 0) ? start_x : 1;
                    int seg_end_x = (core == cores_num - 1 && i == local_segments - 1) ? end_x : vx;
                    VX_SEG *vx_obj = vx_init(iZm_local, seg_start_x, seg_end_x, mpz_get_str(NULL, 10, local_Ys), input_range->mr_rounds);
                    if (!vx_obj)
                    {
                        iZm_free(&iZm_local);
                        mpz_clear(local_Ys);
                        close(pipe_fds[core][1]);
                        exit(1);
                    }

                    vx_full_sieve(vx_obj, 0);
                    child_total += vx_obj->p_count;

                    vx_free(&vx_obj);
                    mpz_add_ui(local_Ys, local_Ys, 1);
                }

                iZm_free(&iZm_local);
                mpz_clear(local_Ys);
            }

            // Send result to parent
            ssize_t written = write(pipe_fds[core][1], &child_total, sizeof(child_total));
            if (written != sizeof(child_total))
            {
                log_error("SiZ_count: Child %d failed to write result.", core);
            }

            close(pipe_fds[core][1]);
            exit(0);
        }
        else
        {
            // Parent process
            close(pipe_fds[core][1]); // close write end
            pipe_fds[core][1] = -1;
        }
    }

    // Collect results
    for (int core = 0; core < cores_num; core++)
    {
        if (pids[core] > 0 && pipe_fds[core][0] != -1)
        {
            uint64_t child_total = 0;
            ssize_t bytes_read = read(pipe_fds[core][0], &child_total, sizeof(child_total));
            if (bytes_read == sizeof(child_total))
            {
                total += child_total;
            }
            else if (bytes_read == 0)
            {
                log_error("SiZ_count: Child %d closed pipe without sending result.", core);
            }
            else if (bytes_read < 0)
            {
                log_error("SiZ_count: Failed to read result from child %d.", core);
            }
            else
            {
                log_error("SiZ_count: Partial read from child %d (got %zd bytes).", core, bytes_read);
            }

            close(pipe_fds[core][0]);
            pipe_fds[core][0] = -1;

            int status;
            waitpid(pids[core], &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {
                log_error("SiZ_count: Child %d exited with status %d.", core, WEXITSTATUS(status));
                // propagate child failure as an overall error
                mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
                iZm_free(&iZm);
                return 0;
            }
            else if (WIFSIGNALED(status))
            {
                log_error("SiZ_count: Child %d terminated by signal %d.", core, WTERMSIG(status));
                mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
                iZm_free(&iZm);
                return 0;
            }
        }
        else if (pipe_fds[core][0] != -1)
        {
            close(pipe_fds[core][0]);
            pipe_fds[core][0] = -1;
        }
    }

    // Cleanup
    mpz_clears(Zs, Ze, Xs, Xe, Ys, Ye, NULL);
    iZm_free(&iZm);

    return total;
}

// =========================================================
// * Random Prime Generation
// =========================================================

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
    compute_max_vx(vx, bit_size);

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

    // 4. Parent reads first result from the pipe.
    close(fd[1]); // Close the write-end.

    if (forks_created == 0)
    {
        log_error("No child processes were created in vy_random_prime, falling back to in-process search");
        close(fd[0]);
        found = vy_search_prime(p, 0, vx);
        mpz_clear(vx);
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

    mpz_clear(vx);

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
