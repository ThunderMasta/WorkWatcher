#include "workwatcher/ui.h"

#include <stdio.h>
#include <time.h>

#include "workwatcher/config.h"

/* Позиционирование курсора в строку N и очистка этой строки. */
#define LINE(n) "\033[" #n ";1H\033[2K"

/* Приглушённый текст подсказки. */
#define DIM   "\033[2m"
#define RESET "\033[0m"

#define LABEL_WORK "Работа"
#define LABEL_HOME "Дом"

/* Возвращает длину результата snprintf, ограниченную размером буфера. */
static size_t clamp_length(int written, size_t size)
{
    if (written < 0) {
        return 0;
    }
    if ((size_t)written >= size) {
        return size > 0 ? size - 1 : 0;
    }
    return (size_t)written;
}

size_t ww_ui_format_duration(double seconds, char *buf, size_t size)
{
    long total = seconds > 0.0 ? (long)seconds : 0L;
    long hours = total / WW_SECONDS_PER_HOUR;
    long minutes = (total % WW_SECONDS_PER_HOUR) / WW_SECONDS_PER_MINUTE;
    long secs = total % WW_SECONDS_PER_MINUTE;

    int written = snprintf(buf, size, "%02ld:%02ld:%02ld", hours, minutes, secs);
    return clamp_length(written, size);
}

size_t ww_ui_format_finish_line(double work_seconds, double now_realtime, char *buf, size_t size)
{
    double remaining = (double)WW_WORKDAY_SECONDS - work_seconds;
    if (remaining <= 0.0) {
        return clamp_length(snprintf(buf, size, "Работа закончена!"), size);
    }

    time_t finish_time = (time_t)(now_realtime + remaining);
    struct tm tm_finish;
    localtime_r(&finish_time, &tm_finish);

    int written = snprintf(buf, size, "Работа закончится: %02d:%02d:%02d", tm_finish.tm_hour,
                           tm_finish.tm_min, tm_finish.tm_sec);
    return clamp_length(written, size);
}

void ww_ui_render(const ww_tracker_t *projected, double now_realtime)
{
    char work_buf[WW_UI_DURATION_BUFSIZE];
    char home_buf[WW_UI_DURATION_BUFSIZE];
    char finish_buf[WW_UI_FINISH_BUFSIZE];

    double work = ww_tracker_elapsed(projected, WW_MODE_WORK);
    double home = ww_tracker_elapsed(projected, WW_MODE_HOME);

    ww_ui_format_duration(work, work_buf, sizeof work_buf);
    ww_ui_format_duration(home, home_buf, sizeof home_buf);
    ww_ui_format_finish_line(work, now_realtime, finish_buf, sizeof finish_buf);

    /* В raw-режиме OPOST отключён, поэтому переводы строк не используются:
       каждая строка адресуется явно. */
    printf(LINE(1) LABEL_WORK ": %s - " LABEL_HOME ": %s" LINE(2) "%s" LINE(3) DIM
           "Любая клавиша или кнопка мыши — сменить режим, Ctrl+C — выход" RESET,
           work_buf, home_buf, finish_buf);
    fflush(stdout);
}

void ww_ui_print_summary(const ww_tracker_t *tracker)
{
    char work_buf[WW_UI_DURATION_BUFSIZE];
    char home_buf[WW_UI_DURATION_BUFSIZE];

    ww_ui_format_duration(ww_tracker_elapsed(tracker, WW_MODE_WORK), work_buf, sizeof work_buf);
    ww_ui_format_duration(ww_tracker_elapsed(tracker, WW_MODE_HOME), home_buf, sizeof home_buf);

    printf("\nИтого — " LABEL_WORK ": %s, " LABEL_HOME ": %s\n", work_buf, home_buf);
    fflush(stdout);
}
