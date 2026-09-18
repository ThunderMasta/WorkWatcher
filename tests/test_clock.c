#include "workwatcher/clock.h"

#include "suites.h"
#include "ww_test.h"

WW_TEST(monotonic_never_decreases)
{
    double a = ww_clock_monotonic_seconds();
    double b = ww_clock_monotonic_seconds();
    WW_ASSERT(b >= a);
}

WW_TEST(realtime_is_plausible)
{
    /* Позже 2020-01-01T00:00:00Z. */
    WW_ASSERT(ww_clock_realtime_seconds() > 1577836800.0);
}

WW_TEST(stable_clock_starts_at_realtime_and_follows_monotonic)
{
    ww_stable_clock_t clock;
    ww_stable_clock_init(&clock);

    double now = ww_stable_clock_now(&clock);
    WW_ASSERT_NEAR(now, clock.anchor_realtime, 0.5);

    /* Подменяем якорь: сдвиг монотонного якоря назад на 100 с должен
       прибавить ровно 100 с к результату — доказательство, что значение
       считается от монотонных часов, а не от системного времени. */
    clock.anchor_monotonic -= 100.0;
    WW_ASSERT_NEAR(ww_stable_clock_now(&clock), now + 100.0, 0.5);
}

void suite_clock(void)
{
    puts("clock");
    WW_RUN(monotonic_never_decreases);
    WW_RUN(realtime_is_plausible);
    WW_RUN(stable_clock_starts_at_realtime_and_follows_monotonic);
}
