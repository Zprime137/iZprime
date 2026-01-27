// utils.h
#ifndef UTILS_H
#define UTILS_H

// necessary includes across files

#include <stdio.h>    // For printf, FILE, fopen, fwrite, fread, etc.
#include <stdlib.h>   // For malloc, free, etc.
#include <stdint.h>   // For fixed-width integer types like uint64_t
#include <stddef.h>   // For size_t
#include <string.h>   // For string manipulation functions like snprintf
#include <assert.h>   // For assertions
#include <math.h>     // For math functions like sqrt
#include <sys/stat.h> // For creating directories (mkdir)
#include <fcntl.h>    // For open
#include <signal.h>   // For kill
#include <unistd.h>   // For close
#include <time.h>     // For time
#include <sys/time.h> // For gettimeofday
#include <sys/wait.h> // For waitpid

// Platform-specific includes for sysctl (macOS/BSD)
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <gmp.h>         // GMP library for arbitrary precision arithmetic
#include <openssl/sha.h> // For SHA-256 hashing

#include <logger.h>     // Logger module for logging timestamped messages.
#include <int_arrays.h> // Integer arrays for holding list of numbers and their metadata.
#include <bitmap.h>     // Bitmap data structure for efficient bit manipulation.

// Output directory
#define DIR_output "./output"           ///< Directory for output files
#define MAX_CORES get_cpu_cores_count() ///< Maximum CPU cores available

// minimum and maximum macros
// #define MIN(a, b) (((a) < (b)) ? (a) : (b))
// #define MAX(a, b) (((a) < (b)) ? (b) : (a))
#define MINMAX(a, b, c) (MIN(MAX(a, b), c))

// file utilities
int create_dir(const char *dir);

// string utilities
int is_numeric_str(const char *str);

// math utilities
uint64_t gcd(uint64_t a, uint64_t b);
uint64_t modular_inverse(uint64_t a, uint64_t m);

// print utilities
void print_line(int length, char fill_char);
void print_centered_text(const char *text, int line_length, char fill_char);
void print_sha256_hash(const unsigned char *hash);

// gmp utilities
void gmp_seed_randstate(gmp_randstate_t state);

// system utilities
int get_cpu_cores_count(void);
int get_cpu_L2_cache_size_kb(void);

// test utils
void print_module_header(char *module_name);
void print_test_header(void);

#endif // UTILS_H
