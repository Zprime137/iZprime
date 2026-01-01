// file playground.c for experimental algorithms

#include <iZ.h>

// SiZ-210 Sieve Algorithm

// space complexity is O(n / 210) bits + output
// returns unordered list of primes up to n
UI64_ARRAY *SiZ_210(uint64_t n)
{
    // Bounds check:
    assert(n > 10 && n <= N_LIMIT && "Input must be in the range (10, 10000000000).");

    // if n is less than 10000, return SiZ(n)
    if (n < 10000)
        return SiZ(n);

    // * 1. Initialization:
    // Initialize primes array with enough capacity to avoid reallocs
    UI64_ARRAY *primes = ui64_init(n / log(n) * 1.4);
    assert(primes && "Memory allocation failed for primes array in SiZm.");

    uint64_t x_n = n / 6 + 1; // max x value up to n
    uint64_t root_limit = sqrt(n) + 1;
    int k = 4; // pointing at 11 in root_primes
    int vx = 35;
    int vy = x_n / vx + 1;

    // Add root primes to primes array
    UI64_ARRAY *root_primes = SiZ(root_limit);
    memcpy(primes->array, root_primes->array, root_primes->count * sizeof(uint64_t));
    primes->count = root_primes->count;

    BITMAP *vy_bitmap = bitmap_init(vy + 8, 1);

    // for each x in [2:vx] such that gcd(iZ(x, ±1), vx) == 1,
    // sieve the corresponding column vy in the iZm space
    for (int x = 2; x <= vx; x++)
    {
        if (gcd(iZ(x, -1), vx) == 1)
        {
            bitmap_set_all(vy_bitmap);
            for (int i = k; i < root_primes->count; i++)
            {
                uint64_t p = root_primes->array[i];
                bitmap_clear_steps_simd(vy_bitmap, p, iZm_solve_for_yp(-1, p, vx, x), vy);
            }

            for (int y = 0; y < vy - 1; y++)
            {
                if (bitmap_get_bit(vy_bitmap, y))
                {
                    uint64_t p = iZ(y * vx + x, -1);
                    ui64_push(primes, p);
                }
            }
            if (bitmap_get_bit(vy_bitmap, vy - 1))
            {
                uint64_t p = iZ((vy - 1) * vx + x, -1);
                if (p < n)
                    ui64_push(primes, p);
            }
        }

        if (gcd(iZ(x, 1), vx) == 1)
        {
            bitmap_set_all(vy_bitmap);
            for (int i = k; i < root_primes->count; i++)
            {
                uint64_t p = root_primes->array[i];
                bitmap_clear_steps_simd(vy_bitmap, p, iZm_solve_for_yp(1, p, vx, x), vy);
            }

            for (int y = 0; y < vy - 1; y++)
            {
                if (bitmap_get_bit(vy_bitmap, y))
                {
                    uint64_t p = iZ(y * vx + x, 1);
                    ui64_push(primes, p);
                }
            }
            if (bitmap_get_bit(vy_bitmap, vy - 1))
            {
                uint64_t p = iZ((vy - 1) * vx + x, 1);
                if (p < n)
                    ui64_push(primes, p);
            }
        }
    }
    ui64_free(&root_primes);
    bitmap_free(&vy_bitmap);

    // Trim excess memory in primes array
    ui64_resize_to_fit(primes);

    return primes;
}

// end of playground.c
