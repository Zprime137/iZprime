/**
 * @file platform.h
 * @brief Cross-platform system abstraction utilities.
 *
 * This header centralizes OS-dependent capabilities used by iZprime.
 * It provides a single layer for process capability flags, entropy,
 * monotonic timing, directory creation, and hardware query helpers.
 */

#ifndef IZ_PLATFORM_H
#define IZ_PLATFORM_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#define IZ_PLATFORM_WINDOWS 1
#define IZ_PLATFORM_POSIX 0
#define IZ_PLATFORM_HAS_FORK 0

#else
#define IZ_PLATFORM_WINDOWS 0
#define IZ_PLATFORM_POSIX 1
#define IZ_PLATFORM_HAS_FORK 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/sysctl.h>
#endif
#endif

/** @defgroup iz_platform Platform Abstraction
 *  @brief OS-adaptation helpers used by utility and API layers.
 *  @{
 */

/**
 * @brief Create a directory if it does not already exist.
 * @param dir Directory path.
 * @return 0 on success, -1 on failure.
 */
int iz_platform_create_dir(const char *dir);

/**
 * @brief Fill a buffer with random bytes.
 * @param buffer Destination buffer.
 * @param bytes Number of bytes to fill.
 * @return 1 on success, 0 on failure.
 */
int iz_platform_fill_random(void *buffer, size_t bytes);

/**
 * @brief Return number of online logical CPU cores (>= 1).
 * @return Core count.
 */
int iz_platform_cpu_cores_count(void);

/**
 * @brief Best-effort L2 cache size query in bits.
 * @return L2 size in bits, or a conservative fallback.
 */
int iz_platform_l2_cache_size_bits(void);

/**
 * @brief Return a monotonic timestamp in seconds.
 * @return Monotonic seconds.
 */
double iz_platform_monotonic_seconds(void);

/** @} */

#endif // IZ_PLATFORM_H
