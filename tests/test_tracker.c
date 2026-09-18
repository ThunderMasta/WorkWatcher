#include "workwatcher/tracker.h"

#include "suites.h"
#include "ww_test.h"

#define EPS 1e-9

WW_TEST(init_zeroes_all_modes)
{
    ww_tracker_t t;
    ww_tracker_init(&t);
    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_WORK), 0.0, EPS);
    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_HOME), 0.0, EPS);
}

WW_TEST(accumulate_adds_to_selected_mode_only)
{
    ww_tracker_t t;
    ww_tracker_init(&t);

    ww_tracker_accumulate(&t, WW_MODE_WORK, 10.5);
    ww_tracker_accumulate(&t, WW_MODE_HOME, 2.0);
    ww_tracker_accumulate(&t, WW_MODE_WORK, 0.5);

    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_WORK), 11.0, EPS);
    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_HOME), 2.0, EPS);
}

WW_TEST(accumulate_ignores_negative_and_invalid)
{
    ww_tracker_t t;
    ww_tracker_init(&t);

    ww_tracker_accumulate(&t, WW_MODE_WORK, -5.0);
    ww_tracker_accumulate(&t, WW_MODE_COUNT, 5.0);
    ww_tracker_accumulate(&t, (ww_mode_t)-1, 5.0);

    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_WORK), 0.0, EPS);
    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_HOME), 0.0, EPS);
    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_COUNT), 0.0, EPS);
}

WW_TEST(project_adds_phase_without_mutating_source)
{
    ww_tracker_t t;
    ww_tracker_init(&t);
    ww_tracker_accumulate(&t, WW_MODE_WORK, 100.0);

    ww_tracker_t p = ww_tracker_project(&t, WW_MODE_HOME, 7.0);

    WW_ASSERT_NEAR(ww_tracker_elapsed(&p, WW_MODE_WORK), 100.0, EPS);
    WW_ASSERT_NEAR(ww_tracker_elapsed(&p, WW_MODE_HOME), 7.0, EPS);
    WW_ASSERT_NEAR(ww_tracker_elapsed(&t, WW_MODE_HOME), 0.0, EPS);
}

WW_TEST(mode_next_cycles)
{
    WW_ASSERT_EQ_INT(ww_mode_next(WW_MODE_WORK), WW_MODE_HOME);
    WW_ASSERT_EQ_INT(ww_mode_next(WW_MODE_HOME), WW_MODE_WORK);
}

WW_TEST(mode_is_valid)
{
    WW_ASSERT(ww_mode_is_valid(WW_MODE_WORK));
    WW_ASSERT(ww_mode_is_valid(WW_MODE_HOME));
    WW_ASSERT(!ww_mode_is_valid(WW_MODE_COUNT));
}

void suite_tracker(void)
{
    puts("tracker");
    WW_RUN(init_zeroes_all_modes);
    WW_RUN(accumulate_adds_to_selected_mode_only);
    WW_RUN(accumulate_ignores_negative_and_invalid);
    WW_RUN(project_adds_phase_without_mutating_source);
    WW_RUN(mode_next_cycles);
    WW_RUN(mode_is_valid);
}
