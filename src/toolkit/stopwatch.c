/**
 * @file stopwatch.c
 * @brief Stopwatch helpers based on monotonic wall-clock time.
 * @ingroup iz_stopwatch
 */

#include <utils.h>

/// @cond IZ_INTERNAL
static struct timespec sw_now_timespec(void)
{
    double now = iz_platform_monotonic_seconds();
    if (now < 0.0)
        now = 0.0;

    struct timespec ts;
    ts.tv_sec = (time_t)now;
    ts.tv_nsec = (long)((now - (double)ts.tv_sec) * 1000000000.0);
    if (ts.tv_nsec < 0)
        ts.tv_nsec = 0;
    if (ts.tv_nsec >= 1000000000L)
        ts.tv_nsec = 999999999L;
    return ts;
}

static double sw_elapsed_between(struct timespec start, struct timespec end)
{
    time_t sec = end.tv_sec - start.tv_sec;
    long nsec = end.tv_nsec - start.tv_nsec;
    if (nsec < 0)
    {
        sec -= 1;
        nsec += 1000000000L;
    }
    return (double)sec + (double)nsec / 1000000000.0;
}
/// @endcond

void sw_start(STOPWATCH *sw)
{
    assert(sw && "Invalid stopwatch passed to sw_start.");
    sw->start_time = sw_now_timespec();
    sw->end_time = sw->start_time;
    sw->running = 1;
}

void sw_stop(STOPWATCH *sw)
{
    assert(sw && "Invalid stopwatch passed to sw_stop.");
    if (!sw->running)
        return;
    sw->end_time = sw_now_timespec();
    sw->running = 0;
    sw->elapsed_sec = sw_elapsed_between(sw->start_time, sw->end_time);
}

double sw_elapsed_seconds(const STOPWATCH *sw)
{
    assert(sw && "Invalid stopwatch passed to sw_elapsed_seconds.");
    if (sw->running)
    {
        struct timespec now = sw_now_timespec();
        return sw_elapsed_between(sw->start_time, now);
    }
    return sw->elapsed_sec;
}

double sw_elapsed_now_seconds(void)
{
    return iz_platform_monotonic_seconds();
}
