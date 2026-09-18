#include "workwatcher/config.h"
#include "workwatcher/ui.h"

#include "suites.h"
#include "ww_test.h"

WW_TEST(duration_formats_hh_mm_ss)
{
    char buf[WW_UI_DURATION_BUFSIZE];

    ww_ui_format_duration(0.0, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "00:00:00");

    ww_ui_format_duration(59.999, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "00:00:59");

    ww_ui_format_duration(3661.0, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "01:01:01");

    ww_ui_format_duration(100.0 * 3600.0, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "100:00:00");
}

WW_TEST(duration_negative_is_zero)
{
    char buf[WW_UI_DURATION_BUFSIZE];
    ww_ui_format_duration(-42.0, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "00:00:00");
}

WW_TEST(duration_returns_length_and_truncates_safely)
{
    char buf[4];
    size_t len = ww_ui_format_duration(3661.0, buf, sizeof buf);
    WW_ASSERT_EQ_INT(len, 3);
    WW_ASSERT_EQ_STR(buf, "01:");

    char big[WW_UI_DURATION_BUFSIZE];
    WW_ASSERT_EQ_INT(ww_ui_format_duration(0.0, big, sizeof big), 8);
}

WW_TEST(finish_line_when_workday_done)
{
    char buf[WW_UI_FINISH_BUFSIZE];
    ww_ui_format_finish_line((double)WW_WORKDAY_SECONDS, 0.0, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "Работа закончена!");

    ww_ui_format_finish_line((double)WW_WORKDAY_SECONDS + 1.0, 0.0, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "Работа закончена!");
}

WW_TEST(finish_line_computes_local_time)
{
    /* TZ=UTC выставлен в test_main.c. now = 10:00:00, отработано 2 часа,
       норма 8 часов → окончание в 16:00:00. */
    char buf[WW_UI_FINISH_BUFSIZE];
    double now = 10.0 * 3600.0;
    ww_ui_format_finish_line(2.0 * 3600.0, now, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "Работа закончится: 16:00:00");
}

WW_TEST(finish_line_rounds_once)
{
    /* Дробные части now и остатка складываются до отбрасывания дроби:
       10:00:00.6 + 5:59:59.6 = 15:60:00.2 → 16:00:00, а не 15:59:59. */
    char buf[WW_UI_FINISH_BUFSIZE];
    double now = 10.0 * 3600.0 + 0.6;
    double worked = (double)WW_WORKDAY_SECONDS - (6.0 * 3600.0 - 0.4);
    ww_ui_format_finish_line(worked, now, buf, sizeof buf);
    WW_ASSERT_EQ_STR(buf, "Работа закончится: 16:00:00");
}

void suite_ui(void)
{
    puts("ui");
    WW_RUN(duration_formats_hh_mm_ss);
    WW_RUN(duration_negative_is_zero);
    WW_RUN(duration_returns_length_and_truncates_safely);
    WW_RUN(finish_line_when_workday_done);
    WW_RUN(finish_line_computes_local_time);
    WW_RUN(finish_line_rounds_once);
}
