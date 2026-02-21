/**
 * @file platform.c
 * @brief Platform abstraction implementation.
 * @ingroup iz_platform
 */

#include <platform.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <openssl/rand.h>

#if IZ_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <direct.h>
#endif

int iz_platform_create_dir(const char *dir)
{
    if (dir == NULL || dir[0] == '\0')
        return -1;

#if IZ_PLATFORM_WINDOWS
    if (_mkdir(dir) == 0 || errno == EEXIST)
        return 0;
#else
    if (mkdir(dir, 0700) == 0 || errno == EEXIST)
        return 0;
#endif
    return -1;
}

int iz_platform_fill_random(void *buffer, size_t bytes)
{
    if (buffer == NULL)
        return 0;

    if (bytes == 0)
        return 1;

    unsigned char *out = (unsigned char *)buffer;
    size_t offset = 0;

    while (offset < bytes)
    {
        size_t chunk = bytes - offset;
        if (chunk > (size_t)INT_MAX)
            chunk = (size_t)INT_MAX;

        if (RAND_bytes(out + offset, (int)chunk) != 1)
            return 0;

        offset += chunk;
    }

    return 1;
}

int iz_platform_cpu_cores_count(void)
{
#if IZ_PLATFORM_WINDOWS
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int cores = (int)info.dwNumberOfProcessors;
    return (cores > 0) ? cores : 1;
#else
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    return (cores > 0) ? (int)cores : 1;
#endif
}

int iz_platform_l2_cache_size_bits(void)
{
#if defined(__linux__)
    FILE *fp = fopen("/sys/devices/system/cpu/cpu0/cache/index2/size", "r");
    if (fp != NULL)
    {
        char buffer[32];
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            int size_kb = 0;
            if (sscanf(buffer, "%dK", &size_kb) == 1 && size_kb > 0)
            {
                fclose(fp);
                return size_kb * 1024 * 8;
            }
        }
        fclose(fp);
    }
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    size_t size_bytes = 0;
    size_t len = sizeof(size_bytes);
    if (sysctlbyname("hw.l2cachesize", &size_bytes, &len, NULL, 0) == 0 && size_bytes > 0)
        return (int)(size_bytes * 8);

    len = sizeof(size_bytes);
    if (sysctlbyname("machdep.cpu.cache.L2_cache_size", &size_bytes, &len, NULL, 0) == 0 && size_bytes > 0)
        return (int)(size_bytes * 8);
#endif

    return 256 * 1024 * 8;
}

double iz_platform_monotonic_seconds(void)
{
#if IZ_PLATFORM_WINDOWS
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency) && QueryPerformanceCounter(&counter) && frequency.QuadPart > 0)
    {
        return (double)counter.QuadPart / (double)frequency.QuadPart;
    }
    return (double)GetTickCount64() / 1000.0;
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;

    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}
