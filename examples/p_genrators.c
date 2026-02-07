#include <iZ_api.h>

#include <string.h>

static void print_usage(const char *prog)
{
    printf("Usage: %s [bit_size] [vx|vy]\n", prog);
    printf("\n");
    printf("Examples:\n");
    printf("  %s 1024 vx\n", prog);
    printf("  %s 2048 vy\n", prog);
    printf("\n");
    printf("Notes:\n");
    printf("- Uses all CPU cores by default.\n");
    printf("- vx/vy select the random prime search strategy.\n");
}

static void example_p_gen(int bit_size, int use_vy)
{
    mpz_t prime;
    mpz_init(prime);
    int cores_num = get_cpu_cores_count();
    printf("Generating a random prime of %d bits using %d cores (%s)...\n", bit_size, cores_num, use_vy ? "vy" : "vx");
    fflush(stdout);

    // use one of the following functions to generate a random prime:
    int result = use_vy ? vy_random_prime(prime, bit_size, cores_num)
                        : vx_random_prime(prime, bit_size, cores_num);

    if (result)
    {
        char *prime_str = mpz_get_str(NULL, 10, prime);
        printf("Generated random prime (%d bits):\n%s\n", bit_size, prime_str);
        free(prime_str);
    }
    else
    {
        printf("Failed to generate random prime.\n");
    }

    mpz_clear(prime);
}

void example_next_prime(void)
{
    mpz_t base, next_prime;
    mpz_init_set_str(base, "1000000000000", 10); // starting point
    mpz_init(next_prime);

    char *base_str = mpz_get_str(NULL, 10, base);
    printf("Finding the next prime after %s...\n", base_str);
    fflush(stdout);

    // get next prime after base
    int result = iZ_next_prime(next_prime, base, 1); // 1 for forward search

    if (result)
    {
        char *next_str = mpz_get_str(NULL, 10, next_prime);
        printf("Next prime is: %s\n", next_str);
        free(next_str);
    }
    else
    {
        printf("Failed to find the next prime.\n");
    }

    // get previous prime before base
    printf("Finding the previous prime before %s...\n", base_str);
    fflush(stdout);

    result = iZ_next_prime(next_prime, base, -1); // -1 for backward search
    if (result)
    {
        char *prev_str = mpz_get_str(NULL, 10, next_prime);
        printf("Previous prime is: %s\n", prev_str);
        free(prev_str);
    }
    else
    {
        printf("Failed to find the previous prime.\n");
    }

    free(base_str);

    mpz_clear(base);
    mpz_clear(next_prime);
}

int main(int argc, char **argv)
{
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        print_usage(argv[0]);
        return 0;
    }

    int bit_size = (argc > 1) ? atoi(argv[1]) : 1024;
    int use_vy = (argc > 2 && strcmp(argv[2], "vy") == 0);

    if (bit_size < 128)
    {
        fprintf(stderr, "bit_size must be >= 128\n");
        return 2;
    }

    example_p_gen(bit_size, use_vy);
    example_next_prime();

    return EXIT_SUCCESS;
}
