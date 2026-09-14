#include "ui.h"

#include <stddef.h>
#include <stdio.h>
#include <time.h>

/* Форматирует длительность как ЧЧ:ММ:СС в буфер. */
static void format_duration(double seconds, char *buf, size_t size)
{
    long total   = (long)seconds;
    long hours   = total / SECONDS_PER_HOUR;
    long minutes = (total % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
    long secs    = total % SECONDS_PER_MINUTE;
    snprintf(buf, size, "%02ld:%02ld:%02ld", hours, minutes, secs);
}

/* Строка о времени окончания рабочего дня. */
static void format_finish_line(double work_seconds, char *buf, size_t size)
{
    double remaining = WORKDAY_SECONDS - work_seconds;
    if (remaining <= 0) {
        snprintf(buf, size, "Работа закончена!");
        return;
    }

    /* Считаем момент окончания как одно непрерывное значение:
       текущее время с долями плюс остаток. Округляем к целым секундам
       только один раз — иначе два независимых округления дают дрожание
       на ±1 секунду. Стабильные настенные часы дополнительно не дают
       скакнуть времени окончания при ручном переводе системных часов. */
    time_t finish_time = (time_t)(stable_realtime_seconds() + remaining);
    struct tm tm_finish;
    localtime_r(&finish_time, &tm_finish);

    snprintf(buf, size, "Работа закончится: %02d:%02d:%02d",
             tm_finish.tm_hour, tm_finish.tm_min, tm_finish.tm_sec);
}

void render(const TimeState *state, Mode mode, double phase_elapsed)
{
    double work = state->work_seconds + (mode == MODE_WORK ? phase_elapsed : 0.0);
    double home = state->home_seconds + (mode == MODE_HOME ? phase_elapsed : 0.0);

    char work_buf[16];
    char home_buf[16];
    char finish_buf[64];

    format_duration(work, work_buf, sizeof work_buf);
    format_duration(home, home_buf, sizeof home_buf);
    format_finish_line(work, finish_buf, sizeof finish_buf);

    printf("\033[1;1H\033[2KРабота: %s - Дом: %s"
           "\033[2;1H\033[2K%s",
           work_buf, home_buf, finish_buf);
    fflush(stdout);
}
