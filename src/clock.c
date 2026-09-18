#include "workwatcher/clock.h"

#include <time.h>

/* Преобразует timespec в секунды с долями. */
static double timespec_to_seconds(const struct timespec *ts)
{
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1e9;
}

double ww_clock_monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return timespec_to_seconds(&ts);
}

double ww_clock_realtime_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return timespec_to_seconds(&ts);
}

void ww_stable_clock_init(ww_stable_clock_t *clock)
{
    clock->anchor_realtime = ww_clock_realtime_seconds();
    clock->anchor_monotonic = ww_clock_monotonic_seconds();
}

double ww_stable_clock_now(const ww_stable_clock_t *clock)
{
    return clock->anchor_realtime + (ww_clock_monotonic_seconds() - clock->anchor_monotonic);
}
