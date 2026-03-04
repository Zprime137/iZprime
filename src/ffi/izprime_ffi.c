/**
 * @file izprime_ffi.c
 * @brief FFI-safe wrapper layer over the native iZprime API.
 */

#define IZP_FFI_BUILD 1
#include <izprime_ffi.h>

#include <iZ_api.h>

#define IZP_MAX_SIEVE_LIMIT 1000000000000ULL
#define IZP_FFI_ERROR_CAP 512

static char g_izp_ffi_last_error[IZP_FFI_ERROR_CAP] = "";

static void izp_ffi_set_error(const char *message)
{
    snprintf(g_izp_ffi_last_error, sizeof(g_izp_ffi_last_error), "%s", message ? message : "");
}

static int izp_ffi_validate_expr_nonnegative(const char *expr, mpz_t out)
{
    if (expr == NULL || expr[0] == '\0')
        return 0;
    if (!parse_numeric_expr_mpz(out, expr))
        return 0;
    return mpz_sgn(out) >= 0;
}

static char *izp_ffi_strdup_heap(const char *src)
{
    size_t len = strlen(src) + 1;
    char *dst = malloc(len);
    if (dst == NULL)
        return NULL;
    memcpy(dst, src, len);
    return dst;
}

static int izp_ffi_copy_mpz_to_string(mpz_t value, char **out_string)
{
    char *tmp = mpz_get_str(NULL, 10, value);
    if (tmp == NULL)
    {
        izp_ffi_set_error("Failed to serialize mpz value to decimal string.");
        return IZP_FFI_ERR_ALLOC;
    }

    *out_string = izp_ffi_strdup_heap(tmp);
    free(tmp);

    if (*out_string == NULL)
    {
        izp_ffi_set_error("Failed to allocate output string.");
        return IZP_FFI_ERR_ALLOC;
    }

    return IZP_FFI_OK;
}

static int izp_ffi_export_primes(UI64_ARRAY *primes, IZP_U64_BUFFER *out)
{
    if (primes == NULL)
    {
        izp_ffi_set_error("Sieve operation failed (NULL prime list).");
        return IZP_FFI_ERR_OPERATION;
    }

    out->data = NULL;
    out->len = 0;

    if (primes->count < 0)
    {
        ui64_free(&primes);
        izp_ffi_set_error("Invalid prime list length returned by core API.");
        return IZP_FFI_ERR_OPERATION;
    }

    size_t len = (size_t)primes->count;
    if (len > 0)
    {
        if (len > SIZE_MAX / sizeof(uint64_t))
        {
            ui64_free(&primes);
            izp_ffi_set_error("Prime list length exceeds addressable memory.");
            return IZP_FFI_ERR_ALLOC;
        }

        out->data = malloc(len * sizeof(uint64_t));
        if (out->data == NULL)
        {
            ui64_free(&primes);
            izp_ffi_set_error("Failed to allocate output prime buffer.");
            return IZP_FFI_ERR_ALLOC;
        }

        memcpy(out->data, primes->array, len * sizeof(uint64_t));
    }

    out->len = len;
    ui64_free(&primes);
    return IZP_FFI_OK;
}

static uint64_t izp_ffi_count_small_range(const mpz_t start, uint64_t range, int mr_rounds)
{
    int rounds = MIN(MAX(mr_rounds, 5), 50);
    uint64_t total = 0;

    mpz_t z;
    mpz_init_set(z, start);

    for (uint64_t i = 0; i < range; i++)
    {
        if (mpz_cmp_ui(z, 2) >= 0 && mpz_probab_prime_p(z, rounds) > 0)
            total++;
        mpz_add_ui(z, z, 1);
    }

    mpz_clear(z);
    return total;
}

const char *izp_ffi_version(void)
{
#ifdef IZ_VERSION
    return IZ_VERSION;
#else
    return "dev";
#endif
}

const char *izp_ffi_status_message(int code)
{
    switch (code)
    {
    case IZP_FFI_OK:
        return "OK";
    case IZP_FFI_ERR_INVALID_ARG:
        return "Invalid argument";
    case IZP_FFI_ERR_PARSE:
        return "Parse error";
    case IZP_FFI_ERR_ALLOC:
        return "Allocation error";
    case IZP_FFI_ERR_NOT_FOUND:
        return "Not found";
    case IZP_FFI_ERR_OPERATION:
        return "Operation failed";
    default:
        return "Unknown status code";
    }
}

const char *izp_ffi_last_error(void)
{
    return g_izp_ffi_last_error;
}

void izp_ffi_clear_error(void)
{
    g_izp_ffi_last_error[0] = '\0';
}

int izp_ffi_sieve_u64(IZP_SIEVE_KIND kind, uint64_t limit, IZP_U64_BUFFER *out)
{
    izp_ffi_clear_error();

    if (out == NULL)
    {
        izp_ffi_set_error("out buffer pointer is NULL.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    out->data = NULL;
    out->len = 0;

    if (limit > IZP_MAX_SIEVE_LIMIT)
    {
        izp_ffi_set_error("Sieve limit must be <= 1000000000000.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    UI64_ARRAY *primes = NULL;

    switch (kind)
    {
    case IZP_SIEVE_SOE:
        primes = SoE(limit);
        break;
    case IZP_SIEVE_SSOE:
        primes = SSoE(limit);
        break;
    case IZP_SIEVE_SOS:
        primes = SoS(limit);
        break;
    case IZP_SIEVE_SSOS:
        primes = SSoS(limit);
        break;
    case IZP_SIEVE_SOEU:
        primes = SoEu(limit);
        break;
    case IZP_SIEVE_SOA:
        primes = SoA(limit);
        break;
    case IZP_SIEVE_SIZ:
        primes = SiZ(limit);
        break;
    case IZP_SIEVE_SIZM:
        primes = SiZm(limit);
        break;
    case IZP_SIEVE_SIZM_VY:
        primes = SiZm_vy(limit);
        break;
    default:
        izp_ffi_set_error("Unknown sieve kind.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    return izp_ffi_export_primes(primes, out);
}

int izp_ffi_count_range(const char *start, uint64_t range, int mr_rounds, int cores_num, uint64_t *out_count)
{
    izp_ffi_clear_error();

    if (out_count == NULL)
    {
        izp_ffi_set_error("out_count pointer is NULL.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    *out_count = 0;

    if (range == 0)
    {
        izp_ffi_set_error("range must be >= 1.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    mpz_t start_value;
    mpz_init(start_value);
    if (!izp_ffi_validate_expr_nonnegative(start, start_value))
    {
        mpz_clear(start_value);
        izp_ffi_set_error("Failed to parse non-negative start expression.");
        return IZP_FFI_ERR_PARSE;
    }

    // Core SiZ_count currently asserts range > 100. Handle tiny ranges directly.
    if (range <= 100)
    {
        *out_count = izp_ffi_count_small_range(start_value, range, mr_rounds);
        mpz_clear(start_value);
        return IZP_FFI_OK;
    }

    char *start_base10 = mpz_get_str(NULL, 10, start_value);
    if (start_base10 == NULL)
    {
        mpz_clear(start_value);
        izp_ffi_set_error("Failed to normalize start expression.");
        return IZP_FFI_ERR_ALLOC;
    }

    INPUT_SIEVE_RANGE input_range = {
        .start = start_base10,
        .range = range,
        .mr_rounds = mr_rounds,
        .filepath = NULL,
        .stream_gaps = 0,
    };

    int worker_count = MAX(cores_num, 1);
    *out_count = SiZ_count(&input_range, worker_count);

    free(start_base10);
    mpz_clear(start_value);
    return IZP_FFI_OK;
}

int izp_ffi_stream_range(const char *start, uint64_t range, int mr_rounds, const char *filepath, int stream_gaps, uint64_t *out_count)
{
    izp_ffi_clear_error();

    if (out_count == NULL)
    {
        izp_ffi_set_error("out_count pointer is NULL.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    *out_count = 0;

    if (filepath == NULL || filepath[0] == '\0')
    {
        izp_ffi_set_error("filepath must be a non-empty string.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    if (range == 0)
    {
        izp_ffi_set_error("range must be >= 1.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    mpz_t start_value;
    mpz_init(start_value);
    if (!izp_ffi_validate_expr_nonnegative(start, start_value))
    {
        mpz_clear(start_value);
        izp_ffi_set_error("Failed to parse non-negative start expression.");
        return IZP_FFI_ERR_PARSE;
    }

    // Probe output path first so path errors map to a deterministic FFI status.
    FILE *probe = fopen(filepath, "w");
    if (probe == NULL)
    {
        mpz_clear(start_value);
        izp_ffi_set_error("Failed to open output path.");
        return IZP_FFI_ERR_OPERATION;
    }
    fclose(probe);

    char *start_base10 = mpz_get_str(NULL, 10, start_value);
    if (start_base10 == NULL)
    {
        mpz_clear(start_value);
        izp_ffi_set_error("Failed to normalize start expression.");
        return IZP_FFI_ERR_ALLOC;
    }

    char *filepath_copy = izp_ffi_strdup_heap(filepath);
    if (filepath_copy == NULL)
    {
        free(start_base10);
        mpz_clear(start_value);
        izp_ffi_set_error("Failed to allocate output path buffer.");
        return IZP_FFI_ERR_ALLOC;
    }

    INPUT_SIEVE_RANGE input_range = {
        .start = start_base10,
        .range = range,
        .mr_rounds = mr_rounds,
        .filepath = filepath_copy,
        .stream_gaps = stream_gaps ? 1 : 0,
    };

    *out_count = SiZ_stream(&input_range);
    free(filepath_copy);
    free(start_base10);
    mpz_clear(start_value);

    return IZP_FFI_OK;
}

int izp_ffi_next_prime(const char *base_expr, int forward, char **out_prime_base10)
{
    izp_ffi_clear_error();

    if (out_prime_base10 == NULL)
    {
        izp_ffi_set_error("out_prime_base10 pointer is NULL.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    *out_prime_base10 = NULL;

    mpz_t base;
    mpz_init(base);
    if (!izp_ffi_validate_expr_nonnegative(base_expr, base))
    {
        mpz_clear(base);
        izp_ffi_set_error("Failed to parse non-negative base expression.");
        return IZP_FFI_ERR_PARSE;
    }

    mpz_t next;
    mpz_init(next);

    int found = iZ_next_prime(next, base, forward ? 1 : 0);
    if (!found)
    {
        mpz_clears(base, next, NULL);
        izp_ffi_set_error("No prime found for the requested direction.");
        return IZP_FFI_ERR_NOT_FOUND;
    }

    int status = izp_ffi_copy_mpz_to_string(next, out_prime_base10);
    mpz_clears(base, next, NULL);
    return status;
}

int izp_ffi_random_prime_vx(int bit_size, int cores_num, char **out_prime_base10)
{
    izp_ffi_clear_error();

    if (out_prime_base10 == NULL)
    {
        izp_ffi_set_error("out_prime_base10 pointer is NULL.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    *out_prime_base10 = NULL;

    if (bit_size < 2)
    {
        izp_ffi_set_error("bit_size must be >= 2.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    mpz_t prime;
    mpz_init(prime);
    int found = vx_random_prime(prime, bit_size, MAX(cores_num, 1));
    if (!found)
    {
        mpz_clear(prime);
        izp_ffi_set_error("vx_random_prime failed.");
        return IZP_FFI_ERR_OPERATION;
    }

    int status = izp_ffi_copy_mpz_to_string(prime, out_prime_base10);
    mpz_clear(prime);
    return status;
}

int izp_ffi_random_prime_vy(int bit_size, int cores_num, char **out_prime_base10)
{
    izp_ffi_clear_error();

    if (out_prime_base10 == NULL)
    {
        izp_ffi_set_error("out_prime_base10 pointer is NULL.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    *out_prime_base10 = NULL;

    if (bit_size < 2)
    {
        izp_ffi_set_error("bit_size must be >= 2.");
        return IZP_FFI_ERR_INVALID_ARG;
    }

    mpz_t prime;
    mpz_init(prime);
    int found = vy_random_prime(prime, bit_size, MAX(cores_num, 1));
    if (!found)
    {
        mpz_clear(prime);
        izp_ffi_set_error("vy_random_prime failed.");
        return IZP_FFI_ERR_OPERATION;
    }

    int status = izp_ffi_copy_mpz_to_string(prime, out_prime_base10);
    mpz_clear(prime);
    return status;
}

void izp_ffi_free_u64_buffer(IZP_U64_BUFFER *buffer)
{
    if (buffer == NULL)
        return;

    free(buffer->data);
    buffer->data = NULL;
    buffer->len = 0;
}

void izp_ffi_free_string(char **str)
{
    if (str == NULL || *str == NULL)
        return;

    free(*str);
    *str = NULL;
}
