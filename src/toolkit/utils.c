/**
 * @file utils.c
 * @brief Implementations for the shared utility layer.
 * @ingroup iz_utils
 */

#include <utils.h>

/**
 * @brief Check if a string is numeric.
 *
 * @param str Pointer to the string.
 * @return int 1 if the string is numeric, 0 otherwise.
 */
int is_numeric_str(const char *str)
{
    if (str == NULL || *str == '\0')
        return 0;

    while (*str)
    {
        if (*str < '0' || *str > '9')
            return 0;
        ++str;
    }
    return 1;
}

/**
 * @brief Print a single line composed of repeated characters.
 * @param length Number of characters to print.
 * @param fill_char Character used to fill the line ('-' when zero).
 */
void print_line(int length, char fill_char)
{
    // if empty fill_char, use '-'
    if (fill_char == '\0')
        fill_char = '-';

    for (int i = 0; i < length; i++)
        printf("%c", fill_char);
    printf("\n");
}

/**
 * @brief Print text centered inside a filled line.
 * @param text Text payload.
 * @param line_length Total line width.
 * @param fill_char Padding character.
 */
void print_centered_text(const char *text, int line_length, char fill_char)
{
    int text_length = strlen(text);
    if (text_length >= line_length)
    {
        printf("%s\n", text);
        return;
    }

    int padding = (line_length - text_length + 1) / 2;

    for (int i = 0; i < padding; i++)
        printf("%c", fill_char);
    printf("%s", text);
    for (int i = 0; i < padding; i++)
        printf("%c", fill_char);
    printf("\n");
}

/**
 * @brief Print the SHA-256 hash in hexadecimal format.
 *
 * @param hash Pointer to the SHA-256 hash array.
 */
void print_sha256_hash(const unsigned char *hash)
{
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        printf("%02x", hash[i]);

    printf("\n");
}

/**
 * @brief Create a directory if it does not exist.
 *
 * @param dir The directory path.
 * @return int 0 if the directory already exists or was successfully created, -1 otherwise.
 */
int create_dir(const char *dir)
{
    struct stat st = {0};
    if (stat(dir, &st) == -1)
    {
        if (mkdir(dir, 0700) != 0)
        {
            // log error if mkdir fails
            log_error("mkdir failed to create %s :\\", dir);
            return -1;
        }
    }

    return 0; // Directory already exists or was successfully created
}

/**
 * @brief Compute the greatest common divisor (GCD) of two integers.
 *
 * @param a The first integer.
 * @param b The second integer.
 * @return int The GCD of a and b.
 */
uint64_t gcd(uint64_t a, uint64_t b)
{
    while (b != 0)
    {
        uint64_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

/**
 * @brief Compute the modular inverse of a modulo m.
 *
 * @description:
 * This function computes the modular inverse of a modulo m using the
 * Extended Euclidean Algorithm. The function takes two integers a and m
 * as input and returns the modular inverse of a modulo m. If a and m
 * are not co-prime, the function returns -1 indicating no modular
 * inverse exists. The function also handles the case where m is 1,
 * returning 0 as the modular inverse.
 *
 * Parameters:
 * @param a          The integer for which the modular inverse is to be computed.
 * @param m          The modulus.
 *
 * @return The modular inverse of a modulo m, or -1 if no inverse exists.
 */
uint64_t modular_inverse(uint64_t a, uint64_t m)
{
    if (m == 1)
        return 0;

    uint64_t m0 = m;
    int64_t x0 = 0, x1 = 1;

    while (a > 1)
    {
        uint64_t q = a / m;
        uint64_t t = m;

        m = a % m;
        a = t;
        t = x0;

        x0 = x1 - (int64_t)q * x0;
        x1 = t;
    }

    if (x1 < 0)
        x1 += (int64_t)m0;
    return (uint64_t)x1;
}

/**
 * @brief Seed the GMP random state.
 *
 * @description: This function seeds the GMP random state using /dev/urandom.
 *
 * @param state The GMP random state.
 */
void gmp_seed_randstate(gmp_randstate_t state)
{
    unsigned long seed;
    int random_fd = open("/dev/urandom", O_RDONLY); // Open /dev/urandom for reading
    if (random_fd == -1)
    {
        // Fallback if /dev/urandom cannot be opened
        seed = (unsigned long)time(NULL);
    }
    else
    {
        read(random_fd, &seed, sizeof(seed)); // Read random bytes from /dev/urandom
        close(random_fd);
    }

    gmp_randseed_ui(state, seed); // Seed the state with the random value
}

/**
 * @brief Compute the number of CPU cores available.
 *
 * @return int The number of CPU cores.
 */
int get_cpu_cores_count(void)
{
    int cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores < 1)
        cores = 1; // Fallback to 1 if unable to determine
    return cores;
}

/**
 * @brief Get the CPU L2 cache size in kilobytes.
 *
 * @description:
 * This function retrieves the L2 cache size using platform-specific methods:
 * - Linux: Reads from sysfs (/sys/devices/system/cpu/cpu0/cache/index2/size)
 * - macOS/BSD: Uses sysctl to query hw.l2cachesize
 * - Fallback: Returns 256 KB as a conservative default
 *
 * The function attempts multiple detection methods to ensure robustness
 * across different architectures and operating systems.
 *
 * @return int The L2 cache size in bits, or 256 KB if unable to determine.
 */
int get_cpu_L2_cache_size_bits(void)
{
    int size_kb = 0;

#ifdef __linux__
    // Linux: Try reading from sysfs
    FILE *fp = fopen("/sys/devices/system/cpu/cpu0/cache/index2/size", "r");
    if (fp != NULL)
    {
        char buffer[32];
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            // Parse size with K suffix (e.g., "256K")
            if (sscanf(buffer, "%dK", &size_kb) == 1)
            {
                fclose(fp);
                return size_kb * 1024 * 8; // Convert KB to bits
            }
        }
        fclose(fp);
    }
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    // macOS and BSD: Use sysctl
    size_t size_bytes = 0;
    size_t len = sizeof(size_bytes);

    // Try hw.l2cachesize (macOS/some BSDs)
    if (sysctlbyname("hw.l2cachesize", &size_bytes, &len, NULL, 0) == 0)
    {
        if (size_bytes > 0)
        {
            return (int)(size_bytes * 8); // Convert bytes to bits
        }
    }

    // Fallback: Try machdep.cpu.cache.L2_cache_size (older macOS)
    if (sysctlbyname("machdep.cpu.cache.L2_cache_size", &size_bytes, &len, NULL, 0) == 0)
    {
        if (size_bytes > 0)
        {
            return (int)(size_bytes * 8); // Convert bytes to bits
        }
    }
#endif

    // Conservative fallback: 256 KB is common for L2 cache per core
    return 256 * 1024 * 8; // Convert 256 KB to bits
}

/**
 * @brief Print module-level banner for test output.
 * @param module_name Display name of the tested module.
 */
void print_module_header(char *module_name)
{
    print_line(60, '*');
    printf("* %s MODULE TEST SUITE\n", module_name);
    print_line(60, '*');
}

/**
 * @brief Print standard test-table column headers.
 */
void print_test_header(void)
{
    print_line(92, '-');
    printf("[%s] %-30s %s %-66s\n", "ID", "Unit Name", "Result", "Details");
    print_line(92, '-');
}
