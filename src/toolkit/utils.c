/**
 * @file utils.c
 * @brief Implementations for the shared utility layer.
 * @ingroup iz_utils
 */

#include <utils.h>
#include <ctype.h>

/// @cond IZ_INTERNAL
static const char *skip_spaces(const char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    return s;
}

static char *dup_trimmed(const char *src)
{
    if (src == NULL)
        return NULL;

    const char *start = skip_spaces(src);
    const char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1]))
        --end;

    size_t len = (size_t)(end - start);
    char *out = malloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static char *normalize_decimal_token(const char *token)
{
    char *trimmed = dup_trimmed(token);
    if (trimmed == NULL)
        return NULL;

    const char *s = trimmed;
    if (*s == '+')
        ++s;

    if (*s == '\0')
    {
        free(trimmed);
        return NULL;
    }

    size_t cap = strlen(s) + 1;
    char *normalized = malloc(cap);
    if (!normalized)
    {
        free(trimmed);
        return NULL;
    }

    size_t n = 0;
    int has_comma = strchr(s, ',') != NULL;

    if (!has_comma)
    {
        for (const char *p = s; *p; ++p)
        {
            if (*p == '_')
                continue;
            if (!isdigit((unsigned char)*p))
            {
                free(normalized);
                free(trimmed);
                return NULL;
            }
            normalized[n++] = *p;
        }
    }
    else
    {
        const char *seg = s;
        int group_idx = 0;
        while (1)
        {
            const char *comma = strchr(seg, ',');
            size_t seg_len = comma ? (size_t)(comma - seg) : strlen(seg);
            if (seg_len == 0)
            {
                free(normalized);
                free(trimmed);
                return NULL;
            }

            if (group_idx == 0)
            {
                if (seg_len < 1 || seg_len > 3)
                {
                    free(normalized);
                    free(trimmed);
                    return NULL;
                }
            }
            else if (seg_len != 3)
            {
                free(normalized);
                free(trimmed);
                return NULL;
            }

            for (size_t i = 0; i < seg_len; ++i)
            {
                if (!isdigit((unsigned char)seg[i]))
                {
                    free(normalized);
                    free(trimmed);
                    return NULL;
                }
                normalized[n++] = seg[i];
            }

            group_idx++;
            if (comma == NULL)
                break;
            seg = comma + 1;
        }
    }

    normalized[n] = '\0';
    free(trimmed);
    return normalized;
}

static int parse_integer_token_mpz(mpz_t out, const char *token)
{
    char *normalized = normalize_decimal_token(token);
    if (normalized == NULL)
        return 0;

    int ok = (mpz_set_str(out, normalized, 10) == 0);
    free(normalized);
    return ok;
}

static int parse_exponent_u64(const char *token, unsigned long *exp_out)
{
    mpz_t exp_mpz;
    mpz_init(exp_mpz);
    int ok = parse_integer_token_mpz(exp_mpz, token);
    if (!ok || !mpz_fits_ulong_p(exp_mpz))
    {
        mpz_clear(exp_mpz);
        return 0;
    }
    *exp_out = mpz_get_ui(exp_mpz);
    mpz_clear(exp_mpz);
    return 1;
}

static int parse_numeric_term_mpz(mpz_t out, const char *term)
{
    char *trimmed = dup_trimmed(term);
    if (trimmed == NULL || trimmed[0] == '\0')
    {
        free(trimmed);
        return 0;
    }

    char *pow_op = strchr(trimmed, '^');
    char *sci_op = strchr(trimmed, 'e');
    if (!sci_op)
        sci_op = strchr(trimmed, 'E');

    if (pow_op && sci_op)
    {
        free(trimmed);
        return 0;
    }

    if (pow_op != NULL)
    {
        if (strchr(pow_op + 1, '^') != NULL || strchr(pow_op + 1, 'e') != NULL || strchr(pow_op + 1, 'E') != NULL)
        {
            free(trimmed);
            return 0;
        }

        *pow_op = '\0';
        const char *base_str = trimmed;
        const char *exp_str = pow_op + 1;

        mpz_t base;
        mpz_init(base);
        unsigned long exp = 0;
        int ok = parse_integer_token_mpz(base, base_str) && parse_exponent_u64(exp_str, &exp);
        if (ok)
            mpz_pow_ui(out, base, exp);

        mpz_clear(base);
        free(trimmed);
        return ok;
    }

    if (sci_op != NULL)
    {
        if (strchr(sci_op + 1, 'e') != NULL || strchr(sci_op + 1, 'E') != NULL || strchr(sci_op + 1, '^') != NULL)
        {
            free(trimmed);
            return 0;
        }

        *sci_op = '\0';
        const char *base_str = trimmed;
        const char *exp_str = sci_op + 1;

        mpz_t base, pow10;
        mpz_inits(base, pow10, NULL);
        unsigned long exp = 0;
        int ok = parse_integer_token_mpz(base, base_str) && parse_exponent_u64(exp_str, &exp);
        if (ok)
        {
            mpz_ui_pow_ui(pow10, 10, exp);
            mpz_mul(out, base, pow10);
        }

        mpz_clears(base, pow10, NULL);
        free(trimmed);
        return ok;
    }

    int ok = parse_integer_token_mpz(out, trimmed);
    free(trimmed);
    return ok;
}

static int parse_range_parts(const char *left_expr, const char *right_expr, mpz_t lower, mpz_t upper)
{
    mpz_t left, right;
    mpz_inits(left, right, NULL);

    int ok = parse_numeric_expr_mpz(left, left_expr) && parse_numeric_expr_mpz(right, right_expr) && mpz_cmp(right, left) >= 0;
    if (ok)
    {
        mpz_set(lower, left);
        mpz_set(upper, right);
    }

    mpz_clears(left, right, NULL);
    return ok;
}
/// @endcond

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

int parse_numeric_expr_mpz(mpz_t out, const char *expr)
{
    if (expr == NULL)
        return 0;

    size_t expr_len = strlen(expr);
    char *copy = malloc(expr_len + 1);
    if (copy == NULL)
        return 0;
    memcpy(copy, expr, expr_len + 1);

    mpz_set_ui(out, 0);
    int has_terms = 0;

    char *cursor = copy;
    while (cursor != NULL)
    {
        char *plus = strchr(cursor, '+');
        if (plus != NULL)
            *plus = '\0';

        mpz_t term_value;
        mpz_init(term_value);
        int ok = parse_numeric_term_mpz(term_value, cursor);
        if (!ok)
        {
            mpz_clear(term_value);
            free(copy);
            return 0;
        }

        mpz_add(out, out, term_value);
        mpz_clear(term_value);
        has_terms = 1;

        cursor = (plus != NULL) ? (plus + 1) : NULL;
    }

    free(copy);
    return has_terms;
}

int parse_numeric_expr_u64(const char *expr, uint64_t *out)
{
    if (out == NULL)
        return 0;

    mpz_t value;
    mpz_init(value);
    if (!parse_numeric_expr_mpz(value, expr) || mpz_sgn(value) < 0 || mpz_sizeinbase(value, 2) > 64)
    {
        mpz_clear(value);
        return 0;
    }

    uint64_t parsed = 0;
    size_t words = 0;
    mpz_export(&parsed, &words, 1, sizeof(parsed), 0, 0, value);
    *out = (words == 0) ? 0 : parsed;
    mpz_clear(value);
    return 1;
}

int parse_inclusive_range_mpz(const char *range_expr, mpz_t lower, mpz_t upper)
{
    if (range_expr == NULL)
        return 0;

    char *range = dup_trimmed(range_expr);
    if (range == NULL || range[0] == '\0')
    {
        free(range);
        return 0;
    }

    size_t len = strlen(range);
    if (len > 7 && strncmp(range, "range[", 6) == 0 && range[len - 1] == ']')
    {
        memmove(range, range + 6, len - 7);
        range[len - 7] = '\0';
    }

    len = strlen(range);
    if (len > 2 && range[0] == '[' && range[len - 1] == ']')
    {
        memmove(range, range + 1, len - 2);
        range[len - 2] = '\0';
    }

    char *sep = strstr(range, "..");
    if (sep != NULL)
    {
        *sep = '\0';
        int ok = parse_range_parts(range, sep + 2, lower, upper);
        free(range);
        return ok;
    }

    sep = strchr(range, ':');
    if (sep != NULL)
    {
        *sep = '\0';
        int ok = parse_range_parts(range, sep + 1, lower, upper);
        free(range);
        return ok;
    }

    for (char *comma = range; (comma = strchr(comma, ',')) != NULL; ++comma)
    {
        char saved = *comma;
        *comma = '\0';
        int ok = parse_range_parts(range, comma + 1, lower, upper);
        *comma = saved;
        if (ok)
        {
            free(range);
            return 1;
        }
    }

    free(range);
    return 0;
}

/**
 * @brief Create a directory if it does not exist.
 *
 * @param dir The directory path.
 * @return int 0 if the directory already exists or was successfully created, -1 otherwise.
 */
int create_dir(const char *dir)
{
    if (iz_platform_create_dir(dir) != 0)
    {
        log_error("Failed to create directory: %s", dir ? dir : "(null)");
        return -1;
    }
    return 0;
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
 * Description:
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
 * Description: This function seeds the GMP random state using platform entropy
 * (OpenSSL-backed), with a time-based fallback.
 *
 * @param state The GMP random state.
 */
void gmp_seed_randstate(gmp_randstate_t state)
{
    unsigned long seed;
    if (!iz_platform_fill_random(&seed, sizeof(seed)))
        seed = (unsigned long)time(NULL);

    gmp_randseed_ui(state, seed);
}

/**
 * @brief Compute the number of CPU cores available.
 *
 * @return int The number of CPU cores.
 */
int get_cpu_cores_count(void)
{
    return iz_platform_cpu_cores_count();
}

/**
 * @brief Get the CPU L2 cache size in bits.
 *
 * Description:
 * This function retrieves the L2 cache size using platform-specific methods:
 * - Linux: Reads from sysfs (/sys/devices/system/cpu/cpu0/cache/index2/size)
 * - macOS/BSD: Uses sysctl to query hw.l2cachesize
 * - Fallback: Returns 256 Kbits as a conservative default
 *
 * The function attempts multiple detection methods to ensure robustness
 * across different architectures and operating systems.
 *
 * @return int The L2 cache size in bits, or 256 Kbits if unable to determine.
 */
int get_cpu_L2_cache_size_bits(void)
{
    return iz_platform_l2_cache_size_bits();
}
