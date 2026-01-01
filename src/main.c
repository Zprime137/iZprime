#include <iZ.h>

void example_sieve(void)
{
    // example using sieve algorithms defined in iZ.h to get an array of primes up to n
    uint64_t n = 1000000000;
    // UI64_ARRAY *primes = SoE(n);
    // UI64_ARRAY *primes = SSoE(n);
    // UI64_ARRAY *primes = SoE(n);
    // UI64_ARRAY *primes = SoS(n);
    // UI64_ARRAY *primes = SoA(n);
    // UI64_ARRAY *primes = SiZ(n);
    UI64_ARRAY *primes = SiZm(n);

    printf("Number of primes up to %llu: %d\n", n, primes->count);
    // print last 10 primes
    printf("Last 10 primes < %llu:\n", n);
    for (int i = primes->count - 10; i < primes->count; i++)
    {
        printf("%llu ", primes->array[i]);
    }
    printf("\n");

    ui64_free(&primes);
}

int main(void)
{
    example_sieve();

    return EXIT_SUCCESS;
}
