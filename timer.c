#include "timer.h"

#include <stdbool.h>
#include <time.h>

double monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

double realtime_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Точка отсчёта для stable_realtime_seconds(): настенное и монотонное
   время, зафиксированные при первом обращении. */
static bool   g_clock_anchored = false;
static double g_anchor_realtime;
static double g_anchor_monotonic;

double stable_realtime_seconds(void)
{
    if (!g_clock_anchored) {
        g_anchor_realtime  = realtime_seconds();
        g_anchor_monotonic = monotonic_seconds();
        g_clock_anchored   = true;
    }

    /* К зафиксированной точке отсчёта прибавляем пройденное монотонное
       время: оно не зависит от перевода системных часов. */
    return g_anchor_realtime + (monotonic_seconds() - g_anchor_monotonic);
}

void accumulate(TimeState *state, Mode mode, double elapsed)
{
    if (mode == MODE_WORK) {
        state->work_seconds += elapsed;
    } else {
        state->home_seconds += elapsed;
    }
}
