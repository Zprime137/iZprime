#include <iZ.h>

void example_p_gen(void)
{
    mpz_t prime;
    mpz_init(prime);
    int bit_size = 1024;
    int cores_num = get_cpu_cores_count();
    printf("Generating a random prime of %d bits using %d cores...\n", bit_size, cores_num);
    fflush(stdout);

    // use one of the following functions to generate a random prime:
    int result = vx_random_prime(prime, bit_size, cores_num);
    // int result = vy_random_prime(prime, bit_size, cores_num);

    if (result)
    {
        printf("Generated random prime (%d bits): %s\n", bit_size, mpz_get_str(NULL, 10, prime));
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

    printf("Finding the next prime after %s...\n", mpz_get_str(NULL, 10, base));
    fflush(stdout);

    // get next prime after base
    int result = iZ_next_prime(next_prime, base, 1); // 1 for forward search

    if (result)
    {
        printf("Next prime is: %s\n", mpz_get_str(NULL, 10, next_prime));
    }
    else
    {
        printf("Failed to find the next prime.\n");
    }

    // get previous prime before base
    printf("Finding the previous prime before %s...\n", mpz_get_str(NULL, 10, base));
    fflush(stdout);

    result = iZ_next_prime(next_prime, base, -1); // -1 for backward search
    if (result)
    {
        printf("Previous prime is: %s\n", mpz_get_str(NULL, 10, next_prime));
    }
    else
    {
        printf("Failed to find the previous prime.\n");
    }

    mpz_clear(base);
    mpz_clear(next_prime);
}

int main(void)
{
    example_p_gen();
    example_next_prime();

    return EXIT_SUCCESS;
}