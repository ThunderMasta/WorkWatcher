#include "workwatcher/tracker.h"

int ww_mode_is_valid(ww_mode_t mode)
{
    /* Сравнение в беззнаковом виде отсекает и отрицательные значения. */
    return (unsigned int)mode < (unsigned int)WW_MODE_COUNT;
}

ww_mode_t ww_mode_next(ww_mode_t mode)
{
    return (mode == WW_MODE_WORK) ? WW_MODE_HOME : WW_MODE_WORK;
}

void ww_tracker_init(ww_tracker_t *tracker)
{
    for (int i = 0; i < WW_MODE_COUNT; ++i) {
        tracker->elapsed[i] = 0.0;
    }
}

void ww_tracker_accumulate(ww_tracker_t *tracker, ww_mode_t mode, double seconds)
{
    if (!ww_mode_is_valid(mode) || seconds < 0.0) {
        return;
    }
    tracker->elapsed[mode] += seconds;
}

double ww_tracker_elapsed(const ww_tracker_t *tracker, ww_mode_t mode)
{
    return ww_mode_is_valid(mode) ? tracker->elapsed[mode] : 0.0;
}

ww_tracker_t ww_tracker_project(const ww_tracker_t *tracker, ww_mode_t active, double phase_seconds)
{
    ww_tracker_t projected = *tracker;
    ww_tracker_accumulate(&projected, active, phase_seconds);
    return projected;
}
