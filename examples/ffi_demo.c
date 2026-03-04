#include <izprime_ffi.h>

#include <inttypes.h>
#include <stdio.h>

int main(void)
{
    IZP_U64_BUFFER primes = {0};

    int status = izp_ffi_sieve_u64(IZP_SIEVE_SIZM, 1000, &primes);
    if (status != IZP_FFI_OK)
    {
        fprintf(stderr, "sieve failed: %s (%s)\n", izp_ffi_status_message(status), izp_ffi_last_error());
        return 1;
    }

    printf("SiZm primes <= 1000: %zu\n", primes.len);
    if (primes.len > 0)
        printf("last=%" PRIu64 "\n", primes.data[primes.len - 1]);

    izp_ffi_free_u64_buffer(&primes);

    char *next = NULL;
    status = izp_ffi_next_prime("10^20 + 12345", 1, &next);
    if (status != IZP_FFI_OK)
    {
        fprintf(stderr, "next_prime failed: %s (%s)\n", izp_ffi_status_message(status), izp_ffi_last_error());
        return 1;
    }

    printf("next prime: %s\n", next);
    izp_ffi_free_string(&next);

    return 0;
}
