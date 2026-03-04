/**
 * @file cli_app.c
 * @brief CLI application dispatcher and command handlers.
 */

#include <cli.h>
#include <iZ_api.h>
#include <inttypes.h>
#include <limits.h>
#include <openssl/crypto.h>
#include <time.h>

#ifndef IZ_VERSION
#define IZ_VERSION "dev"
#endif

typedef UI64_ARRAY *(*SIEVE_FN)(uint64_t);
typedef int (*CLI_HANDLER)(int argc, char **argv);

typedef struct
{
    const char *name;
    SIEVE_FN fn;
} CLI_SIEVE_MODEL;

static const CLI_SIEVE_MODEL k_sieve_models[] = {
    {"SoE", SoE},
    {"SSoE", SSoE},
    {"SoEu", SoEu},
    {"SoS", SoS},
    {"SoA", SoA},
    {"SiZ", SiZ},
    {"SiZm", SiZm},
};

static const size_t k_sieve_models_count = sizeof(k_sieve_models) / sizeof(k_sieve_models[0]);

typedef struct
{
    const char *name;
    CLI_HANDLER handler;
} CLI_COMMAND;

static int run_directional_prime_cmd(int argc, char **argv, int forward);

static int parse_expr_u64(const char *value, uint64_t *out)
{
    return parse_numeric_expr_u64(value, out);
}

static int parse_expr_int(const char *value, int *out)
{
    uint64_t parsed = 0;
    if (!parse_numeric_expr_u64(value, &parsed) || parsed > INT_MAX)
        return 0;
    *out = (int)parsed;
    return 1;
}

static int parse_cores_value(const char *value, int *out)
{
    if (value == NULL || out == NULL)
        return 0;

    if (strcmp(value, "max") == 0)
    {
        *out = get_cpu_cores_count();
        return 1;
    }

    if (!parse_expr_int(value, out) || *out < 1)
        return 0;

    return 1;
}

static int read_cli_option_value(
    int argc,
    char **argv,
    int *index,
    const char **value,
    const char *option_name)
{
    if (*index + 1 >= argc)
    {
        fprintf(stderr, "Missing value after %s.\n", option_name);
        return 0;
    }

    *index += 1;
    *value = argv[*index];
    return 1;
}

static char *dup_cli_string(const char *src)
{
    if (src == NULL)
        return NULL;

    size_t len = strlen(src) + 1;
    char *dst = malloc(len);
    if (dst == NULL)
        return NULL;

    memcpy(dst, src, len);
    return dst;
}

typedef struct
{
    char lower[1024];
    char upper[1024];
    uint64_t range_size;
} CLI_RANGE;

static int parse_cli_range(const char *value, CLI_RANGE *range)
{
    if (range == NULL)
        return 0;

    mpz_t lower;
    mpz_t upper;
    mpz_t width;
    mpz_inits(lower, upper, width, NULL);

    int ok = parse_inclusive_range_mpz(value, lower, upper);
    if (!ok)
    {
        mpz_clears(lower, upper, width, NULL);
        return 0;
    }

    mpz_sub(width, upper, lower);
    mpz_add_ui(width, width, 1);
    if (mpz_sgn(width) <= 0 || mpz_sizeinbase(width, 2) > 64)
    {
        mpz_clears(lower, upper, width, NULL);
        return 0;
    }

    size_t words = 0;
    uint64_t span = 0;
    mpz_export(&span, &words, 1, sizeof(span), 0, 0, width);
    if (words == 0 || span == 0)
    {
        mpz_clears(lower, upper, width, NULL);
        return 0;
    }

    char *lower_str = mpz_get_str(NULL, 10, lower);
    char *upper_str = mpz_get_str(NULL, 10, upper);
    if (lower_str == NULL || upper_str == NULL)
    {
        free(lower_str);
        free(upper_str);
        mpz_clears(lower, upper, width, NULL);
        return 0;
    }

    if (strlen(lower_str) >= sizeof(range->lower) || strlen(upper_str) >= sizeof(range->upper))
    {
        free(lower_str);
        free(upper_str);
        mpz_clears(lower, upper, width, NULL);
        return 0;
    }

    snprintf(range->lower, sizeof(range->lower), "%s", lower_str);
    snprintf(range->upper, sizeof(range->upper), "%s", upper_str);
    range->range_size = span;

    free(lower_str);
    free(upper_str);
    mpz_clears(lower, upper, width, NULL);
    return 1;
}

static void print_general_help(const char *prog)
{
    printf("iZprime CLI\n");
    printf("Usage: %s <command> [options]\n\n", prog);
    printf("Commands:\n");
    printf("  stream_primes  Stream primes over a range (uses SiZ_stream)\n");
    printf("  count_primes   Count primes over a range (uses SiZ_count)\n");
    printf("  next_prime     Find the next prime after n (uses iZ_next_prime)\n");
    printf("  prev_prime     Find the previous prime before n (uses iZ_next_prime)\n");
    printf("  is_prime       Check primality for n (uses test_primality)\n");
    printf("  test           Run API-level consistency tests\n");
    printf("  benchmark      Benchmark sieve models\n");
    printf("  doctor         Check runtime environment and dependencies\n");
    printf("  help           Show this message\n\n");
    printf("Aliases: sieve -> stream_primes, count -> count_primes, prev -> prev_prime\n");
    printf("Use '%s <command> --help' for command-specific options.\n", prog);
}

static void print_stream_help(const char *prog)
{
    printf("Usage: %s stream_primes --range \"[LOWER, UPPER]\" [--print | --stream-to FILE] [--print-gaps] [--mr-rounds N]\n", prog);
    printf("Notes:\n");
    printf("  - Range is inclusive and accepts large-number expressions.\n");
    printf("  - Supported numeric operators: + - * / ^ e and parentheses.\n");
    printf("  - If no output option is set, output defaults to output/stream_<timestamp>.txt\n");
    printf("  - --print-gaps emits prime gaps from segment base (implies --print).\n");
}

static void print_count_help(const char *prog)
{
    printf("Usage: %s count_primes --range \"[LOWER, UPPER]\" [--cores N|max] [--mr-rounds N]\n", prog);
    printf("Notes:\n");
    printf("  - Range is inclusive and accepts large-number expressions.\n");
    printf("  - --cores accepts an integer >= 1 or the literal 'max'.\n");
    printf("  - core count is clamped to available CPU cores.\n");
    printf("  - --cores-number is accepted as a backward-compatible alias.\n");
}

static void print_next_prime_help(const char *prog)
{
    printf("Usage: %s next_prime --n VALUE\n", prog);
    printf("Notes:\n");
    printf("  - VALUE accepts the same numeric expression syntax as range bounds.\n");
}

static void print_prev_prime_help(const char *prog)
{
    printf("Usage: %s prev_prime --n VALUE\n", prog);
    printf("Notes:\n");
    printf("  - VALUE accepts the same numeric expression syntax as range bounds.\n");
}

static void print_is_prime_help(const char *prog)
{
    printf("Usage: %s is_prime --n VALUE [--rounds N]\n", prog);
    printf("Notes:\n");
    printf("  - --rounds defaults to %d.\n", MR_ROUNDS);
}

static void print_test_help(const char *prog)
{
    printf("Usage: %s test [--limit N]\n", prog);
    printf("Notes:\n");
    printf("  - Runs cross-model consistency checks against SoE.\n");
}

static void print_benchmark_help(const char *prog)
{
    printf("Usage: %s benchmark [--limit N] [--repeat N] [--algo NAME|all] [--save-results FILE]\n", prog);
    printf("Supported models:\n");
    for (size_t i = 0; i < k_sieve_models_count; ++i)
    {
        printf("  - %s\n", k_sieve_models[i].name);
    }
}

typedef struct
{
    int has_range;
    CLI_RANGE range;
    int mr_rounds;
    int print_to_console;
    int print_gaps;
    const char *stream_path;
} STREAM_CMD_OPTIONS;

typedef enum
{
    STREAM_PARSE_OK = 0,
    STREAM_PARSE_HELP = 1,
    STREAM_PARSE_ERROR = 2,
} STREAM_PARSE_RESULT;

static int stream_read_option_value(
    int argc,
    char **argv,
    int *index,
    const char **value,
    const char *option)
{
    return read_cli_option_value(argc, argv, index, value, option);
}

static STREAM_PARSE_RESULT parse_stream_range_option(
    int argc,
    char **argv,
    int *index,
    STREAM_CMD_OPTIONS *options)
{
    const char *value = NULL;
    if (!stream_read_option_value(argc, argv, index, &value, "--range"))
        return STREAM_PARSE_ERROR;

    if (!parse_cli_range(value, &options->range))
    {
        fprintf(stderr, "Invalid --range value. Expected [LOWER, UPPER] with LOWER<=UPPER.\n");
        return STREAM_PARSE_ERROR;
    }

    options->has_range = 1;
    return STREAM_PARSE_OK;
}

static STREAM_PARSE_RESULT parse_stream_output_option(
    int argc,
    char **argv,
    int *index,
    STREAM_CMD_OPTIONS *options,
    const char *option)
{
    const char *value = NULL;
    if (!stream_read_option_value(argc, argv, index, &value, option))
        return STREAM_PARSE_ERROR;
    options->stream_path = value;
    return STREAM_PARSE_OK;
}

static STREAM_PARSE_RESULT parse_stream_rounds_option(
    int argc,
    char **argv,
    int *index,
    STREAM_CMD_OPTIONS *options)
{
    const char *value = NULL;
    if (!stream_read_option_value(argc, argv, index, &value, "--mr-rounds"))
        return STREAM_PARSE_ERROR;

    if (!parse_expr_int(value, &options->mr_rounds))
    {
        fprintf(stderr, "Invalid --mr-rounds value.\n");
        return STREAM_PARSE_ERROR;
    }

    return STREAM_PARSE_OK;
}

static STREAM_PARSE_RESULT parse_stream_primes_option(
    int argc,
    char **argv,
    int *index,
    STREAM_CMD_OPTIONS *options)
{
    const char *arg = argv[*index];

    if (strcmp(arg, "--range") == 0)
        return parse_stream_range_option(argc, argv, index, options);
    if (strcmp(arg, "--print") == 0)
    {
        options->print_to_console = 1;
        return STREAM_PARSE_OK;
    }
    if (strcmp(arg, "--print-gaps") == 0)
    {
        options->print_gaps = 1;
        options->print_to_console = 1;
        return STREAM_PARSE_OK;
    }
    if (strcmp(arg, "--stream_to") == 0 || strcmp(arg, "--stream-to") == 0)
        return parse_stream_output_option(argc, argv, index, options, arg);
    if (strcmp(arg, "--mr-rounds") == 0)
        return parse_stream_rounds_option(argc, argv, index, options);

    fprintf(stderr, "Unknown option: %s\n", arg);
    return STREAM_PARSE_ERROR;
}

static STREAM_PARSE_RESULT parse_stream_primes_args(int argc, char **argv, STREAM_CMD_OPTIONS *options)
{
    STREAM_PARSE_RESULT result = STREAM_PARSE_OK;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_stream_help(argv[0]);
            result = STREAM_PARSE_HELP;
            break;
        }
        result = parse_stream_primes_option(argc, argv, &i, options);
        if (result != STREAM_PARSE_OK)
            break;
    }

    return result;
}

static int validate_stream_primes_options(const STREAM_CMD_OPTIONS *options)
{
    if (!options->has_range)
    {
        fprintf(stderr, "Missing required option: --range \"[LOWER, UPPER]\"\n");
        return EXIT_FAILURE;
    }
    if (options->print_gaps && options->stream_path != NULL)
    {
        fprintf(stderr, "--print-gaps cannot be combined with --stream-to.\n");
        return EXIT_FAILURE;
    }
    if (options->print_to_console && options->stream_path != NULL)
    {
        fprintf(stderr, "Use either --print or --stream-to, not both.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static const char *resolve_stream_path(const STREAM_CMD_OPTIONS *options, char *default_path, size_t default_path_size)
{
    if (options->print_to_console)
        return NULL;
    if (options->stream_path != NULL)
        return options->stream_path;

    time_t now = time(NULL);
    struct tm tm_now;
    char stamp[64];

    int tm_ok = iz_platform_localtime(&now, &tm_now);
    if (!tm_ok || strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tm_now) == 0)
        snprintf(stamp, sizeof(stamp), "unknown");

    snprintf(default_path, default_path_size, "%s/stream_%s.txt", DIR_output, stamp);
    return default_path;
}

static int run_stream_primes_cmd(int argc, char **argv)
{
    STREAM_CMD_OPTIONS options = {0};
    options.mr_rounds = 25;

    STREAM_PARSE_RESULT parse_result = parse_stream_primes_args(argc, argv, &options);
    if (parse_result == STREAM_PARSE_HELP)
        return EXIT_SUCCESS;
    if (parse_result == STREAM_PARSE_ERROR)
        return EXIT_FAILURE;

    if (validate_stream_primes_options(&options) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    create_dir(DIR_output);
    char default_path[256];
    const char *stream_path = resolve_stream_path(&options, default_path, sizeof(default_path));
    char *stream_path_mut = dup_cli_string(stream_path);
    if (stream_path != NULL && stream_path_mut == NULL)
    {
        fprintf(stderr, "Failed to allocate stream path buffer.\n");
        return EXIT_FAILURE;
    }

    INPUT_SIEVE_RANGE input = {
        .start = options.range.lower,
        .range = options.range.range_size,
        .mr_rounds = options.mr_rounds,
        .stream_gaps = options.print_gaps,
        // NULL filepath tells SiZ_stream() to emit directly to stdout.
        .filepath = stream_path_mut};

    STOPWATCH timer;
    sw_start(&timer);
    uint64_t count = SiZ_stream(&input);
    sw_stop(&timer);
    free(stream_path_mut);
    printf("\n");

    if (!options.print_to_console)
        printf("Streamed primes to: %s\n", stream_path);

    printf("Prime count in [%s, %s] = %" PRIu64 "\n", options.range.lower, options.range.upper, count);
    printf("Elapsed (s): %.6f\n", timer.elapsed_sec);
    return EXIT_SUCCESS;
}

static int run_count_primes_cmd(int argc, char **argv)
{
    int has_range = 0;
    CLI_RANGE range = {0};
    int mr_rounds = 25;
    int cores = get_cpu_cores_count();

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_count_help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--range") == 0)
        {
            const char *range_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &range_value, "--range"))
                return EXIT_FAILURE;
            if (!parse_cli_range(range_value, &range))
            {
                fprintf(stderr, "Invalid --range value. Expected [LOWER, UPPER] with LOWER<=UPPER.\n");
                return EXIT_FAILURE;
            }
            has_range = 1;
            continue;
        }
        if (strcmp(argv[i], "--cores") == 0 || strcmp(argv[i], "--cores-number") == 0)
        {
            const char *cores_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &cores_value, argv[i]))
                return EXIT_FAILURE;
            if (!parse_cores_value(cores_value, &cores))
            {
                fprintf(stderr, "Invalid --cores value. Use an integer >= 1 or 'max'.\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strcmp(argv[i], "--mr-rounds") == 0)
        {
            const char *rounds_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &rounds_value, "--mr-rounds"))
                return EXIT_FAILURE;
            if (!parse_expr_int(rounds_value, &mr_rounds))
            {
                fprintf(stderr, "Invalid --mr-rounds value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return EXIT_FAILURE;
    }

    if (!has_range)
    {
        fprintf(stderr, "Missing required option: --range \"[LOWER, UPPER]\"\n");
        return EXIT_FAILURE;
    }

    if (range.range_size <= 100)
    {
        fprintf(stderr, "Range size must be > 100 for SiZ_count preconditions.\n");
        return EXIT_FAILURE;
    }

    INPUT_SIEVE_RANGE input = {
        .start = range.lower,
        .range = range.range_size,
        .mr_rounds = mr_rounds,
        .stream_gaps = 0,
        .filepath = NULL,
    };

    STOPWATCH timer;
    sw_start(&timer);
    uint64_t count = SiZ_count(&input, cores);
    sw_stop(&timer);

    printf("Prime count in [%s, %s] = %" PRIu64 "\n", range.lower, range.upper, count);
    printf("Cores used: %d\n", MIN(cores, get_cpu_cores_count()));
    printf("Elapsed (s): %.6f\n", timer.elapsed_sec);
    return EXIT_SUCCESS;
}

static int run_test_cmd(int argc, char **argv)
{
    uint64_t limit = 1000000ULL;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_test_help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--limit") == 0)
        {
            const char *limit_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &limit_value, "--limit"))
                return EXIT_FAILURE;
            if (!parse_expr_u64(limit_value, &limit))
            {
                fprintf(stderr, "Invalid --limit value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return EXIT_FAILURE;
    }

    if (limit <= 10)
    {
        fprintf(stderr, "--limit must be > 10.\n");
        return EXIT_FAILURE;
    }

    printf("Running consistency test for n=%" PRIu64 "\n", limit);
    UI64_ARRAY *baseline = SoE(limit);
    if (!baseline)
    {
        fprintf(stderr, "SoE baseline failed.\n");
        return EXIT_FAILURE;
    }

    int failures = 0;
    printf("[PASS] SoE count=%d\n", baseline->count);

    for (size_t i = 1; i < k_sieve_models_count; ++i)
    {
        UI64_ARRAY *arr = k_sieve_models[i].fn(limit);
        if (!arr)
        {
            printf("[FAIL] %s returned NULL\n", k_sieve_models[i].name);
            failures++;
            continue;
        }

        int ok = (arr->count == baseline->count);
        if (ok)
        {
            for (int j = 0; j < baseline->count; ++j)
            {
                if (arr->array[j] != baseline->array[j])
                {
                    ok = 0;
                    break;
                }
            }
        }

        if (ok)
            printf("[PASS] %s\n", k_sieve_models[i].name);
        else
        {
            printf("[FAIL] %s mismatch against SoE\n", k_sieve_models[i].name);
            failures++;
        }
        ui64_free(&arr);
    }

    ui64_free(&baseline);
    if (failures != 0)
    {
        printf("Test failures: %d\n", failures);
        return EXIT_FAILURE;
    }

    printf("All model consistency tests passed.\n");
    return EXIT_SUCCESS;
}

static int run_benchmark_cmd(int argc, char **argv)
{
    uint64_t limit = 10000000ULL;
    int repeat = 3;
    const char *algo = "all";
    const char *save_path = NULL;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_benchmark_help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--limit") == 0)
        {
            const char *limit_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &limit_value, "--limit"))
                return EXIT_FAILURE;
            if (!parse_expr_u64(limit_value, &limit))
            {
                fprintf(stderr, "Invalid --limit value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strcmp(argv[i], "--repeat") == 0)
        {
            const char *repeat_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &repeat_value, "--repeat"))
                return EXIT_FAILURE;
            if (!parse_expr_int(repeat_value, &repeat) || repeat < 1)
            {
                fprintf(stderr, "Invalid --repeat value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strcmp(argv[i], "--algo") == 0)
        {
            const char *algo_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &algo_value, "--algo"))
                return EXIT_FAILURE;
            algo = algo_value;
            continue;
        }
        if (strcmp(argv[i], "--save-results") == 0)
        {
            const char *save_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &save_value, "--save-results"))
                return EXIT_FAILURE;
            save_path = save_value;
            continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return EXIT_FAILURE;
    }

    FILE *out = NULL;
    if (save_path != NULL)
    {
        out = fopen(save_path, "w");
        if (!out)
        {
            fprintf(stderr, "Failed to open %s\n", save_path);
            return EXIT_FAILURE;
        }
        fprintf(out, "algorithm,limit,repeat,avg_seconds,prime_count\n");
    }
    printf("Benchmark: n=%" PRIu64 ", repeat=%d\n", limit, repeat);
    printf("%-10s %-12s %-14s %-12s\n", "Model", "Primes", "Avg(s)", "Status");

    int failures = 0;
    int matched_models = 0;
    for (size_t i = 0; i < k_sieve_models_count; ++i)
    {
        if (strcmp(algo, "all") != 0 && strcmp(algo, k_sieve_models[i].name) != 0)
            continue;
        matched_models++;

        double total_seconds = 0.0;
        int prime_count = -1;
        int ok = 1;

        for (int r = 0; r < repeat; ++r)
        {
            STOPWATCH timer;
            sw_start(&timer);
            UI64_ARRAY *arr = k_sieve_models[i].fn(limit);
            sw_stop(&timer);
            double elapsed = timer.elapsed_sec;
            if (!arr)
            {
                ok = 0;
                break;
            }

            total_seconds += elapsed;
            prime_count = arr->count;
            ui64_free(&arr);
        }

        if (!ok)
        {
            printf("%-10s %-12s %-14s %-12s\n", k_sieve_models[i].name, "-", "-", "FAIL");
            failures++;
            continue;
        }

        double avg = total_seconds / (double)repeat;
        printf("%-10s %-12d %-14.6f %-12s\n", k_sieve_models[i].name, prime_count, avg, "OK");
        if (out != NULL)
            fprintf(out, "%s,%" PRIu64 ",%d,%.6f,%d\n", k_sieve_models[i].name, limit, repeat, avg, prime_count);
    }

    if (save_path != NULL)
    {
        fclose(out);
        printf("Saved benchmark results: %s\n", save_path);
    }

    if (matched_models == 0)
    {
        fprintf(stderr, "Unknown model '%s'. Use --help to list supported names.\n", algo);
        return EXIT_FAILURE;
    }

    if (failures != 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

static int run_next_prime_cmd(int argc, char **argv)
{
    return run_directional_prime_cmd(argc, argv, 1);
}

static int run_directional_prime_cmd(int argc, char **argv, int forward)
{
    const char *value_expr = NULL;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            if (forward)
                print_next_prime_help(argv[0]);
            else
                print_prev_prime_help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--n") == 0)
        {
            const char *n_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &n_value, "--n"))
                return EXIT_FAILURE;
            value_expr = n_value;
            continue;
        }
        if (argv[i][0] != '-' && value_expr == NULL)
        {
            value_expr = argv[i];
            continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return EXIT_FAILURE;
    }

    if (value_expr == NULL)
    {
        fprintf(stderr, "Missing required value. Use --n VALUE.\n");
        return EXIT_FAILURE;
    }

    mpz_t base;
    mpz_t prime;
    mpz_inits(base, prime, NULL);
    if (!parse_numeric_expr_mpz(base, value_expr))
    {
        fprintf(stderr, "Invalid numeric expression for --n.\n");
        mpz_clears(base, prime, NULL);
        return EXIT_FAILURE;
    }

    STOPWATCH timer;
    sw_start(&timer);
    int found = iZ_next_prime(prime, base, forward);
    sw_stop(&timer);

    if (!found)
    {
        fprintf(stderr, "Failed to find the %s prime.\n", forward ? "next" : "previous");
        mpz_clears(base, prime, NULL);
        return EXIT_FAILURE;
    }

    if (forward)
        gmp_printf("Next prime after %Zd is %Zd\n", base, prime);
    else
        gmp_printf("Previous prime before %Zd is %Zd\n", base, prime);
    printf("Elapsed (s): %.6f s\n", timer.elapsed_sec);

    mpz_clears(base, prime, NULL);
    return EXIT_SUCCESS;
}

static int run_prev_prime_cmd(int argc, char **argv)
{
    return run_directional_prime_cmd(argc, argv, 0);
}

static int run_is_prime_cmd(int argc, char **argv)
{
    const char *value_expr = NULL;
    int rounds = MR_ROUNDS;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_is_prime_help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--n") == 0)
        {
            const char *n_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &n_value, "--n"))
                return EXIT_FAILURE;
            value_expr = n_value;
            continue;
        }
        if (strcmp(argv[i], "--rounds") == 0)
        {
            const char *rounds_value = NULL;
            if (!read_cli_option_value(argc, argv, &i, &rounds_value, "--rounds"))
                return EXIT_FAILURE;
            if (!parse_expr_int(rounds_value, &rounds) || rounds < 1)
            {
                fprintf(stderr, "Invalid --rounds value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (argv[i][0] != '-' && value_expr == NULL)
        {
            value_expr = argv[i];
            continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return EXIT_FAILURE;
    }

    if (value_expr == NULL)
    {
        fprintf(stderr, "Missing required value. Use --n VALUE.\n");
        return EXIT_FAILURE;
    }

    mpz_t n;
    mpz_init(n);
    if (!parse_numeric_expr_mpz(n, value_expr))
    {
        fprintf(stderr, "Invalid numeric expression for --n.\n");
        mpz_clear(n);
        return EXIT_FAILURE;
    }

    STOPWATCH timer;
    sw_start(&timer);
    int result = test_primality(n, rounds);
    sw_stop(&timer);

    if (result > 0)
    {
        const char *certainty = (result == 2) ? "definitely prime" : "probably prime";
        gmp_printf("%Zd is prime (%s)\n", n, certainty);
    }
    else
    {
        gmp_printf("%Zd is composite\n", n);
    }
    printf("Elapsed (s): %.6f s\n", timer.elapsed_sec);

    mpz_clear(n);
    return EXIT_SUCCESS;
}

static int run_doctor_cmd(int argc, char **argv)
{
    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf("Usage: %s doctor\n", argv[0]);
            return EXIT_SUCCESS;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;
    printf("iZprime doctor\n");
    printf("Version: %s\n", IZ_VERSION);
    printf("CPU cores: %d\n", get_cpu_cores_count());
    printf("GMP version: %s\n", gmp_version);
    printf("OpenSSL: %s\n", OpenSSL_version(OPENSSL_VERSION));

    if (create_dir(DIR_output) != 0)
    {
        printf("[FAIL] Cannot create output directory: %s\n", DIR_output);
        status = EXIT_FAILURE;
    }
    else
    {
        printf("[PASS] Output directory is writable: %s\n", DIR_output);
    }

    return status;
}

static const CLI_COMMAND k_commands[] = {
    {"stream_primes", run_stream_primes_cmd},
    {"sieve", run_stream_primes_cmd},
    {"count_primes", run_count_primes_cmd},
    {"count", run_count_primes_cmd},
    {"next_prime", run_next_prime_cmd},
    {"prev_prime", run_prev_prime_cmd},
    {"prev", run_prev_prime_cmd},
    {"is_prime", run_is_prime_cmd},
    {"test", run_test_cmd},
    {"benchmark", run_benchmark_cmd},
    {"doctor", run_doctor_cmd},
};

static const CLI_COMMAND *parse_command(const char *name)
{
    size_t count = sizeof(k_commands) / sizeof(k_commands[0]);
    for (size_t i = 0; i < count; ++i)
    {
        if (strcmp(name, k_commands[i].name) == 0)
            return &k_commands[i];
    }
    return NULL;
}

int cli_run(int argc, char **argv)
{
    log_set_log_level(LOG_WARNING);
    create_dir(DIR_output);

    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    {
        print_general_help(argv[0]);
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "help") == 0)
    {
        if (argc == 2)
        {
            print_general_help(argv[0]);
            return EXIT_SUCCESS;
        }

        const CLI_COMMAND *help_cmd = parse_command(argv[2]);
        if (help_cmd == NULL)
        {
            fprintf(stderr, "Unknown command: %s\n\n", argv[2]);
            print_general_help(argv[0]);
            return EXIT_FAILURE;
        }

        char help_name[64];
        snprintf(help_name, sizeof(help_name), "%s", help_cmd->name);
        char help_flag[] = "--help";
        char *help_argv[] = {argv[0], help_name, help_flag};
        return help_cmd->handler(3, help_argv);
    }

    const CLI_COMMAND *cmd = parse_command(argv[1]);
    if (cmd != NULL)
        return cmd->handler(argc, argv);

    fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
    print_general_help(argv[0]);
    return EXIT_FAILURE;
}
