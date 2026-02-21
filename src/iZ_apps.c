/**
 * @file iZ_apps.c
 * @brief High-level application routines built on top of the iZ_toolkit.
 *
 * This module contains range-oriented entry points (stream/count) and
 * probabilistic prime generation wrappers.
 * @ingroup iz_api
 */

#include <iZ_api.h>
#include <inttypes.h>

#if IZ_PLATFORM_HAS_FORK
static int write_all_bytes(int fd, const char *buf, size_t len)
{
    size_t total = 0;
    while (total < len)
    {
        ssize_t written = write(fd, buf + total, len - total);
        if (written <= 0)
            return 0;
        total += (size_t)written;
    }
    return 1;
}
#endif

// =========================================================
// * SiZ Range Variants
// =========================================================

/**
 * @ingroup iz_api
 * @brief Stream primes in an arbitrary numeric range using iZ toolkit.
 *
 * This function counts and streams primes in the interval [Zs, Ze] defined by the input range.
 * It maps the numeric bounds into the iZ index space (6x ± 1), partitions that space
 * into y segments of length vx, and streams primes in each segment using the iZ toolkit.
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

    int has_output_file = (input_range->filepath && input_range->filepath[0] != '\0');
    FILE *output = stdout; // default to stdout if no valid filepath is provided

    if (has_output_file)
    {
        output = fopen(input_range->filepath, "w");
        if (output == NULL)
        {
            log_error("Failed to open output file: %s", input_range->filepath);
            return 0;
        }
    }

    uint64_t total = 0; // output: total prime count

    int vx = VX6; // Use VX6 segment size (1,616,615 bits) for optimal results
    // Miller-Rabin rounds, bounded [5, 50]
    int mr_rounds = MIN(MAX(input_range->mr_rounds, 5), 50);

    IZM_RANGE_INFO info = range_info_init(input_range, vx);
    if (info.y_range < 0)
    {
        if (has_output_file)
            fclose(output);
        range_info_free(&info);
        return 0;
    }

    IZM *iZm = NULL;
    mpz_t current_y;
    mpz_init(current_y);
    mpz_set(current_y, info.Ys);
    int start_x = mpz_fdiv_ui(info.Xs, info.vx);
    int end_x = mpz_fdiv_ui(info.Xe, info.vx);

    // if Ys = 0, use SiZm for the first segment
    if (mpz_cmp_ui(current_y, 0) == 0)
    {
        uint64_t limit = mpz_cmp_ui(info.Ye, 0) > 0 ? info.vx : end_x;
        UI64_ARRAY *primes = SiZm(limit * 6 + 1);
        if (!primes)
        {
            total = 0;
            goto stream_cleanup;
        }

        uint64_t s = mpz_get_ui(info.Zs);
        uint64_t e = mpz_get_ui(info.Ze);

        for (int i = 0; i < primes->count; i++)
        {
            // only primes in [Zs, Ze]
            if (primes->array[i] > s && primes->array[i] <= e)
            {
                total++;
                fprintf(output, "%" PRIu64 " ", primes->array[i]);
            }
        }

        start_x = 1;                         // next segment starts at x=1
        mpz_add_ui(current_y, current_y, 1); // increment Ys for the next segment

        ui64_free(&primes);
    }

    // No remaining vx segments after the initial one.
    if (mpz_cmp(current_y, info.Ye) > 0)
    {
        goto stream_cleanup;
    }

    // Initialize iZm structure for vx segments
    iZm = iZm_init(vx);
    if (!iZm)
    {
        total = 0;
        goto stream_cleanup;
    }

    // Process remaining segments for y in [current_y:Ye]
    int first_segment = 1;
    while (mpz_cmp(current_y, info.Ye) <= 0)
    {
        int seg_start_x = first_segment ? start_x : 1;
        int seg_end_x = (mpz_cmp(current_y, info.Ye) == 0) ? end_x : vx;
        char *y_str = mpz_get_str(NULL, 10, current_y);
        if (!y_str)
        {
            total = 0;
            goto stream_cleanup;
        }

        VX_SEG *vx_obj = vx_init(iZm, seg_start_x, seg_end_x, y_str, mr_rounds);
        free(y_str);
        if (!vx_obj)
        {
            // check logs for errors
            total = 0;
            goto stream_cleanup;
        }

        vx_stream(vx_obj, output);
        total += vx_obj->p_count; // accumulate prime count

        vx_free(&vx_obj);
        first_segment = 0;
        mpz_add_ui(current_y, current_y, 1); // increment Ys for the next segment
    }

stream_cleanup:
    iZm_free(&iZm);
    range_info_free(&info);
    mpz_clear(current_y);
    if (has_output_file)
        fclose(output);
    else
        fflush(stdout);

    return total;
}

/**
 * @ingroup iz_api
 * @brief Multi-process prime counting over an arbitrary numeric range using iZ toolkit.
 *
 * This function parallelizes prime counting over the interval [Zs, Ze]
 * (interpreted from @p input_range) by partitioning the corresponding iZ
 * index space into VX segments and distributing contiguous blocks of segments
 * across multiple processes. Each child process returns a local prime count to
 * the parent via a pipe.
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
    int vx = compute_l2_vx(pow(10, 9)); // Use a segment width that balances workload and overhead for 10^9 range
    cores_num = MAX(1, MIN(cores_num, get_cpu_cores_count()));
#if !IZ_PLATFORM_HAS_FORK
    if (cores_num > 1)
    {
        log_info("SiZ_count: multi-process mode is unavailable on this platform; using single-process mode.");
        cores_num = 1;
    }
#endif
    IZM *iZm = NULL;
#if IZ_PLATFORM_HAS_FORK
    int (*pipe_fds)[2] = NULL;
    pid_t *pids = NULL;
#endif

    IZM_RANGE_INFO info = range_info_init(input_range, vx);
    if (info.y_range < 0)
    {
        range_info_free(&info);
        return 0;
    }

    mpz_t current_y;
    mpz_init(current_y);
    mpz_set(current_y, info.Ys);
    int start_x = mpz_fdiv_ui(info.Xs, vx);
    int end_x = mpz_fdiv_ui(info.Xe, vx);

    // if current_y = 0, use sieve_iZm for the first segment
    if (mpz_cmp_ui(current_y, 0) == 0)
    {
        uint64_t limit = mpz_cmp_ui(info.Ye, 0) > 0 ? vx : end_x;
        UI64_ARRAY *primes = SiZm(limit * 6 + 1);
        if (!primes)
        {
            total = 0;
            goto count_cleanup;
        }

        total += primes->count;
        uint64_t s = mpz_get_ui(info.Zs);
        uint64_t e = mpz_get_ui(info.Ze);

        for (int i = 0; i < primes->count; i++)
        {
            // Exclude values outside [Zs, Ze] from the first solid segment.
            if (primes->array[i] < s || primes->array[i] > e)
            {
                total--;
            }
        }

        ui64_free(&primes);
        start_x = 1;
        mpz_add_ui(current_y, current_y, 1); // increment Ys for the next segment
    }

    if (mpz_cmp(current_y, info.Ye) > 0)
    {
        goto count_cleanup;
    }

    iZm = iZm_init(vx);
    if (!iZm)
    {
        total = 0;
        goto count_cleanup;
    }

    // Handle edge cases:
    mpz_t prime_z;
    mpz_init(prime_z);
    // if Ys > 0 and Zs % 6 <= 1
    if (mpz_cmp_ui(current_y, 0) > 0 && mpz_fdiv_ui(info.Zs, 6) <= 1)
    {
        // if iZ(Xs, -1) < Zs and prime, decrement total
        iZ_mpz(prime_z, info.Xs, -1);
        if (mpz_cmp(prime_z, info.Zs) < 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }
    // if Ye > 0 and Ze % 6 <= 1
    if (mpz_cmp_ui(info.Ye, 0) > 0 && mpz_fdiv_ui(info.Ze, 6) <= 1)
    {
        // if iZ(Xe, 1) > Ze and prime, decrement total
        iZ_mpz(prime_z, info.Xe, 1);
        if (mpz_cmp(prime_z, info.Ze) > 0)
        {
            if (mpz_probab_prime_p(prime_z, 25))
            {
                total--;
            }
        }
    }
    mpz_clear(prime_z);

    int total_segments = (int)(mpz_get_ui(info.Ye) - mpz_get_ui(current_y) + 1);
    if (total_segments <= 0)
    {
        goto count_cleanup;
    }

    // Single-process processing of all segments
    if (cores_num == 1)
    {
        int first_segment = 1;
        for (int i = 0; i < total_segments; i++)
        {
            int seg_start_x = first_segment ? start_x : 1;
            int seg_end_x = (i == total_segments - 1) ? end_x : vx;
            char *y_str = mpz_get_str(NULL, 10, current_y);
            if (!y_str)
            {
                total = 0;
                goto count_cleanup;
            }

            VX_SEG *vx_obj = vx_init(iZm, seg_start_x, seg_end_x, y_str, input_range->mr_rounds);
            free(y_str);
            if (!vx_obj)
            {
                total = 0;
                goto count_cleanup;
            }

            vx_full_sieve(vx_obj, 0);
            total += vx_obj->p_count;

            vx_free(&vx_obj);
            first_segment = 0;
            mpz_add_ui(current_y, current_y, 1); // increment Ys for the next segment
        }
        goto count_cleanup;
    }

#if IZ_PLATFORM_HAS_FORK
    // Multi-process processing of remaining segments
    // if total_segments < cores_num, adjust cores_num
    if (total_segments < cores_num)
    {
        cores_num = total_segments;
    }

    int segments_per_core = total_segments / cores_num;
    int remainder_segments = total_segments % cores_num;

    pipe_fds = malloc((size_t)cores_num * sizeof(*pipe_fds));
    pids = malloc((size_t)cores_num * sizeof(*pids));
    if (!pipe_fds || !pids)
    {
        log_error("SiZ_count: Failed to allocate process bookkeeping arrays.");
        total = 0;
        goto count_cleanup;
    }

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
            total = 0;
            goto count_cleanup;
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
            total = 0;
            goto count_cleanup;
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
                mpz_set(local_Ys, current_y);
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
                    int global_segment = offset + i;
                    int seg_start_x = (global_segment == 0) ? start_x : 1;
                    int seg_end_x = (global_segment == total_segments - 1) ? end_x : vx;
                    char *y_str = mpz_get_str(NULL, 10, local_Ys);
                    if (!y_str)
                    {
                        iZm_free(&iZm_local);
                        mpz_clear(local_Ys);
                        close(pipe_fds[core][1]);
                        exit(1);
                    }

                    VX_SEG *vx_obj = vx_init(iZm_local, seg_start_x, seg_end_x, y_str, input_range->mr_rounds);
                    free(y_str);
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
                total = 0;
                goto count_cleanup;
            }
            else if (WIFSIGNALED(status))
            {
                log_error("SiZ_count: Child %d terminated by signal %d.", core, WTERMSIG(status));
                total = 0;
                goto count_cleanup;
            }
        }
        else if (pipe_fds[core][0] != -1)
        {
            close(pipe_fds[core][0]);
            pipe_fds[core][0] = -1;
        }
    }
#endif

count_cleanup:
#if IZ_PLATFORM_HAS_FORK
    free(pids);
    free(pipe_fds);
#endif
    range_info_free(&info);
    iZm_free(&iZm);
    mpz_clear(current_y);

    return total;
}

// =========================================================
// * Random Prime Generation
// =========================================================

/**
 * @ingroup iz_api
 * @brief Generates a random prime candidate using the vy_search_prime routine.
 *
 * Description: This function generates a random prime of a given bit size using the vy_search_prime routine.
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

#if !IZ_PLATFORM_HAS_FORK
    log_info("vy_random_prime: multi-process mode is unavailable on this platform; using single-process mode.");
    found = vy_search_prime(p, 0, vx);
    mpz_clear(vx);
    return found;
#else
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
                    if (!write_all_bytes(fd[1], p_str, strlen(p_str) + 1))
                        log_warn("Failed to write prime candidate from child process in vy_random_prime");
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
#endif
}

/**
 * @ingroup iz_api
 * @brief Generates a random prime candidate using the vx_search_prime routine.
 *
 * Description: This function generates a random prime of a given bit size using the vx_search_prime routine.
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

#if !IZ_PLATFORM_HAS_FORK
    log_info("vx_random_prime: multi-process mode is unavailable on this platform; using single-process mode.");
    found = vx_search_prime(p, 0, vx, bit_size);
    return found;
#else
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
                    if (!write_all_bytes(fd[1], p_str, strlen(p_str) + 1))
                        log_warn("Failed to write prime candidate from child process in vx_random_prime");
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
#endif
}

/**
 * @ingroup iz_api
 * @brief Find the next prime number after a given base.
 *
 * Description: This function searches for the next/previous prime number after a given base using the iZ framework.
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
