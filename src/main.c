/**
 * @file main.c
 * @brief CLI entry point for the iZprime library.
 */

#include <iZ_api.h>
#include <inttypes.h>
#include <limits.h>
#include <openssl/crypto.h>

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

    mpz_t lower, upper, width;
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
    printf("  is_prime       Check primality for n (uses check_primality)\n");
    printf("  test           Run API-level consistency tests\n");
    printf("  benchmark      Benchmark sieve models\n");
    printf("  doctor         Check runtime environment and dependencies\n");
    printf("  help           Show this message\n\n");
    printf("Aliases: sieve -> stream_primes, count -> count_primes\n");
    printf("Use '%s <command> --help' for command-specific options.\n", prog);
}

static void print_stream_help(const char *prog)
{
    printf("Usage: %s stream_primes --range \"[LOWER, UPPER]\" [--print | --stream-to FILE] [--mr-rounds N]\n", prog);
    printf("Notes:\n");
    printf("  - Range is inclusive and accepts large-number expressions.\n");
    printf("  - Supported numeric forms: 10^6, 1e6, 1,000,000, 10e100 + 10e9\n");
    printf("  - If no output option is set, output defaults to output/stream_<timestamp>.txt\n");
}

static void print_count_help(const char *prog)
{
    printf("Usage: %s count_primes --range \"[LOWER, UPPER]\" [--cores-number N] [--mr-rounds N]\n", prog);
    printf("Notes:\n");
    printf("  - Range is inclusive and accepts large-number expressions.\n");
    printf("  - --cores-number is clamped to available CPU cores.\n");
}

static void print_next_prime_help(const char *prog)
{
    printf("Usage: %s next_prime --n VALUE\n", prog);
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

static int run_stream_primes_cmd(int argc, char **argv)
{
    int has_range = 0;
    CLI_RANGE range = {0};
    int mr_rounds = 25;
    int print_to_console = 0;
    const char *stream_path = NULL;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_stream_help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--range") == 0)
        {
            if (i + 1 >= argc || !parse_cli_range(argv[++i], &range))
            {
                fprintf(stderr, "Invalid --range value. Expected [LOWER, UPPER] with LOWER<=UPPER.\n");
                return EXIT_FAILURE;
            }
            has_range = 1;
            continue;
        }
        if (strcmp(argv[i], "--print") == 0)
        {
            print_to_console = 1;
            continue;
        }
        if (strcmp(argv[i], "--stream_to") == 0 || strcmp(argv[i], "--stream-to") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Missing filepath after %s.\n", argv[i]);
                return EXIT_FAILURE;
            }
            stream_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--mr-rounds") == 0)
        {
            if (i + 1 >= argc || !parse_expr_int(argv[++i], &mr_rounds))
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
    if (print_to_console && stream_path != NULL)
    {
        fprintf(stderr, "Use either --print or --stream-to, not both.\n");
        return EXIT_FAILURE;
    }

    create_dir(DIR_output);
    char default_path[256];
    if (!print_to_console && stream_path == NULL)
    {
        time_t now = time(NULL);
        struct tm *tm_now = localtime(&now);
        char stamp[64];
        strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", tm_now);
        snprintf(default_path, sizeof(default_path), "%s/stream_%s.txt", DIR_output, stamp);
        stream_path = default_path;
    }

    INPUT_SIEVE_RANGE input = {
        .start = range.lower,
        .range = range.range_size,
        .mr_rounds = mr_rounds,
        .filepath = (char *)(print_to_console ? "/dev/stdout" : stream_path)};

    STOPWATCH timer;
    sw_start(&timer);
    uint64_t count = SiZ_stream(&input);
    sw_stop(&timer);
    printf("\n");

    if (!print_to_console)
        printf("Streamed primes to: %s\n", stream_path);

    printf("Prime count in [%s, %s] = %" PRIu64 "\n", range.lower, range.upper, count);
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
            if (i + 1 >= argc || !parse_cli_range(argv[++i], &range))
            {
                fprintf(stderr, "Invalid --range value. Expected [LOWER, UPPER] with LOWER<=UPPER.\n");
                return EXIT_FAILURE;
            }
            has_range = 1;
            continue;
        }
        if (strcmp(argv[i], "--cores-number") == 0 || strcmp(argv[i], "--cores") == 0)
        {
            if (i + 1 >= argc || !parse_expr_int(argv[++i], &cores) || cores < 1)
            {
                fprintf(stderr, "Invalid --cores-number value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strcmp(argv[i], "--mr-rounds") == 0)
        {
            if (i + 1 >= argc || !parse_expr_int(argv[++i], &mr_rounds))
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
            if (i + 1 >= argc || !parse_expr_u64(argv[++i], &limit))
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
            if (i + 1 >= argc || !parse_expr_u64(argv[++i], &limit))
            {
                fprintf(stderr, "Invalid --limit value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strcmp(argv[i], "--repeat") == 0)
        {
            if (i + 1 >= argc || !parse_expr_int(argv[++i], &repeat) || repeat < 1)
            {
                fprintf(stderr, "Invalid --repeat value.\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strcmp(argv[i], "--algo") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Missing value after --algo.\n");
                return EXIT_FAILURE;
            }
            algo = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--save-results") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Missing filepath after --save-results.\n");
                return EXIT_FAILURE;
            }
            save_path = argv[++i];
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
    const char *value_expr = NULL;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_next_prime_help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--n") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Missing value after --n.\n");
                return EXIT_FAILURE;
            }
            value_expr = argv[++i];
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

    mpz_t base, prime;
    mpz_inits(base, prime, NULL);
    if (!parse_numeric_expr_mpz(base, value_expr))
    {
        fprintf(stderr, "Invalid numeric expression for --n.\n");
        mpz_clears(base, prime, NULL);
        return EXIT_FAILURE;
    }

    STOPWATCH timer;
    sw_start(&timer);
    int found = iZ_next_prime(prime, base, 1);
    sw_stop(&timer);

    if (!found)
    {
        fprintf(stderr, "Failed to find the next prime.\n");
        mpz_clears(base, prime, NULL);
        return EXIT_FAILURE;
    }

    gmp_printf("Next prime after %Zd is %Zd\n", base, prime);
    printf("Elapsed (s): %.6f s\n", timer.elapsed_sec);

    mpz_clears(base, prime, NULL);
    return EXIT_SUCCESS;
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
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Missing value after --n.\n");
                return EXIT_FAILURE;
            }
            value_expr = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--rounds") == 0)
        {
            if (i + 1 >= argc || !parse_expr_int(argv[++i], &rounds) || rounds < 1)
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
    int result = check_primality(n, rounds);
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

int main(int argc, char **argv)
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

        char *help_argv[] = {argv[0], (char *)help_cmd->name, "--help"};
        return help_cmd->handler(3, help_argv);
    }

    const CLI_COMMAND *cmd = parse_command(argv[1]);
    if (cmd != NULL)
        return cmd->handler(argc, argv);

    fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
    print_general_help(argv[0]);
    return EXIT_FAILURE;
}
