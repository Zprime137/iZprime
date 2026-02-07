/**
 * @file sieve_primes.c
 * @brief Example CLI for running different sieve algorithms.
 */

#include <iZ_api.h>

#include <string.h>

typedef UI64_ARRAY *(*sieve_fn)(uint64_t);

typedef struct
{
    const char *name;
    sieve_fn fn;
} SIEVE_ENTRY;

static const SIEVE_ENTRY SIEVES[] = {
    {"SoE", SoE},
    {"SSoE", SSoE},
    {"SiZ", SiZ},
    {"SiZm", SiZm},
    {"SiZm_vy", SiZm_vy}, // unordered output (fast); OK for counting
};

static const int SIEVES_COUNT = sizeof(SIEVES) / sizeof(SIEVES[0]);

static void print_usage(const char *prog)
{
    printf("Usage: %s [algo] [limit] [print_last]\n", prog);
    printf("\n");
    printf("algo: one of: ");
    for (int i = 0; i < SIEVES_COUNT; i++)
    {
        printf("%s%s", SIEVES[i].name, (i + 1 == SIEVES_COUNT) ? "" : ", ");
    }
    printf("\n");
    printf("limit: integer upper bound (default: 1000000)\n");
    printf("print_last: how many primes to print from the end (default: 10)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s SiZm 10000000 10\n", prog);
    printf("  %s SiZm_vy 10000000 0\n", prog);
}

static const SIEVE_ENTRY *find_sieve(const char *name)
{
    for (int i = 0; i < SIEVES_COUNT; i++)
    {
        if (strcmp(SIEVES[i].name, name) == 0)
            return &SIEVES[i];
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        print_usage(argv[0]);
        return 0;
    }

    const char *algo = (argc > 1) ? argv[1] : "SiZm";
    uint64_t limit = (argc > 2) ? (uint64_t)strtoull(argv[2], NULL, 10) : 1000000ULL;
    int print_last = (argc > 3) ? atoi(argv[3]) : 10;

    const SIEVE_ENTRY *sieve = find_sieve(algo);
    if (!sieve)
    {
        fprintf(stderr, "Unknown algo '%s'\n\n", algo);
        print_usage(argv[0]);
        return 2;
    }

    if (limit < 10)
    {
        fprintf(stderr, "limit must be >= 10\n");
        return 2;
    }

    UI64_ARRAY *primes = sieve->fn(limit);
    if (!primes)
    {
        fprintf(stderr, "Failed to generate primes using %s\n", sieve->name);
        return 1;
    }

    printf("Algorithm:  %s\n", sieve->name);
    printf("Limit:      %llu\n", (unsigned long long)limit);
    printf("Count:      %d\n", primes->count);
    if (primes->count > 0)
        printf("Last prime: %llu\n", (unsigned long long)primes->array[primes->count - 1]);

    if (print_last > 0)
    {
        int start = primes->count - print_last;
        if (start < 0)
            start = 0;

        printf("\nLast %d primes (order depends on algo):\n", primes->count - start);
        for (int i = start; i < primes->count; i++)
        {
            printf("%llu%s", (unsigned long long)primes->array[i], (i + 1 == primes->count) ? "\n" : " ");
        }
    }

    ui64_free(&primes);
    return 0;
}
