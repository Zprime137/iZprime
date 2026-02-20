#include <iZ_api.h>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

static void ensure_dir(const char *path)
{
    if (!path || !path[0])
        return;

    if (mkdir(path, 0755) != 0 && errno != EEXIST)
    {
        fprintf(stderr, "Failed to create directory '%s': %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void print_usage(const char *prog)
{
    printf("Usage: %s [start] [range] [output_file]\n", prog);
    printf("\n");
    printf("Examples:\n");
    printf("  %s 0 1000000\n", prog);
    printf("  %s 1000000000000 1000000 output/iZ_stream.txt\n", prog);
    printf("\n");
    printf("Notes:\n");
    printf("- If output_file is provided, primes are streamed to that file.\n");
    printf("- If output_file is omitted, this example only counts primes in the range.\n");
}

static void run_SiZ_stream(const char *start_str, uint64_t range, const char *filepath)
{
    STOPWATCH timer;
    double elapsed_seconds;
    uint64_t prime_count;

    if (filepath)
        ensure_dir("output");

    INPUT_SIEVE_RANGE input_range = {
        .start = (char *)start_str,
        .range = range,
        .mr_rounds = MR_ROUNDS,
        .filepath = (char *)filepath,
    };

    sw_start(&timer);
    prime_count = SiZ_stream(&input_range);
    sw_stop(&timer);
    elapsed_seconds = sw_elapsed_seconds(&timer);

    print_line(30, '=');
    printf("Start:               %s\n", start_str);
    printf("Range:               %-16llu\n", (unsigned long long)range);
    printf("Primes in range:      %-16llu\n", (unsigned long long)prime_count);
    if (filepath)
        printf("Output file:          %s\n", filepath);
    printf("Execution time (s):   %-16f\n", elapsed_seconds);
    fflush(stdout);
}

static void run_SiZ_count(const char *start_str, uint64_t range)
{
    STOPWATCH timer;
    double elapsed_seconds;
    uint64_t prime_count;

    INPUT_SIEVE_RANGE input_range = {
        .start = (char *)start_str,
        .range = range,
        .mr_rounds = MR_ROUNDS,
        .filepath = NULL,
    };
    int cores_num = get_cpu_cores_count();

    sw_start(&timer);
    prime_count = SiZ_count(&input_range, cores_num);
    sw_stop(&timer);
    elapsed_seconds = sw_elapsed_seconds(&timer);

    print_line(30, '=');
    printf("Start:               %s\n", start_str);
    printf("Range:               %-16llu\n", (unsigned long long)range);
    printf("Primes in range:      %-16llu\n", (unsigned long long)prime_count);
    printf("Cores:               %-16d\n", cores_num);
    printf("Execution time (s):   %-16f\n", elapsed_seconds);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        print_usage(argv[0]);
        return 0;
    }

    const char *start_str = (argc > 1) ? argv[1] : "0";
    uint64_t range = (argc > 2) ? (uint64_t)strtoull(argv[2], NULL, 10) : 1000000ULL;
    const char *filepath = (argc > 3) ? argv[3] : NULL;

    if (range == 0)
    {
        fprintf(stderr, "Range must be > 0\n");
        return 2;
    }

    if (filepath)
        run_SiZ_stream(start_str, range, filepath);
    else
        run_SiZ_count(start_str, range);

    return EXIT_SUCCESS;
}
