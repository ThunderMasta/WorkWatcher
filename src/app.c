#include "workwatcher/app.h"

#include "workwatcher/config.h"
#include "workwatcher/terminal.h"
#include "workwatcher/ui.h"

void ww_app_init(ww_app_t *app)
{
    ww_tracker_init(&app->tracker);
    ww_stable_clock_init(&app->clock);
    app->mode = WW_MODE_WORK;
}

/*
 * Одна фаза: перерисовывает экран, пока не придёт событие, требующее
 * реакции, затем засчитывает накопленное время в счётчик активного режима.
 * Возвращает полученное событие (TOGGLE или QUIT).
 */
static ww_input_event_t run_phase(ww_app_t *app)
{
    double phase_start = ww_clock_monotonic_seconds();
    ww_input_event_t event = WW_INPUT_NONE;

    while (event == WW_INPUT_NONE) {
        double phase_elapsed = ww_clock_monotonic_seconds() - phase_start;
        ww_tracker_t projected = ww_tracker_project(&app->tracker, app->mode, phase_elapsed);
        ww_ui_render(&projected, app->mode, ww_stable_clock_now(&app->clock));
        event = ww_ui_poll_input(WW_REFRESH_INTERVAL_US);
    }

    ww_tracker_accumulate(&app->tracker, app->mode, ww_clock_monotonic_seconds() - phase_start);
    return event;
}

void ww_app_run(ww_app_t *app)
{
    for (;;) {
        ww_input_event_t event = run_phase(app);
        if (event == WW_INPUT_QUIT) {
            break;
        }
        app->mode = ww_mode_next(app->mode);
    }

    /* Сначала снимаем ncurses, затем аварийный restore termios — иначе
       сводка попадёт в альтернативный экран. */
    ww_ui_shutdown();
    ww_terminal_restore();
    ww_ui_print_summary(&app->tracker);
}
