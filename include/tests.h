#ifndef TESTS_H
#define TESTS_H

#include <iZ.h>

// ========================================================================
// * Testing and Benchmarking Sieve Algorithms
// ========================================================================
int TEST_SIEVE_MODELS_INTEGRITY(int verbose);
void BENCHMARK_SIEVE_MODELS(int save_results);

// ========================================================================
// * Testing and Benchmarking Sieve-iZ-Range Algorithms
// ========================================================================
int TEST_SiZ_stream(int verbose);
int TEST_SiZ_count(int verbose);

// ========================================================================
// * Testing and Benchmarking Random Prime Generation Algorithms
// ========================================================================
int TEST_iZ_next_prime(int verbose);
int TEST_vy_random_prime(int verbose);
int TEST_vx_random_prime(int verbose);

int BENCHMARK_P_GEN_ALGORITHMS(int bit_size, int test_rounds, int save_results);

#endif // TESTS_H
