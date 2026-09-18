#include "workwatcher/app.h"

#include "workwatcher/config.h"
#include "workwatcher/input.h"
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
    unsigned char input[WW_INPUT_BUFFER_SIZE];
    double phase_start = ww_clock_monotonic_seconds();
    ww_input_event_t event = WW_INPUT_NONE;

    while (event == WW_INPUT_NONE) {
        double phase_elapsed = ww_clock_monotonic_seconds() - phase_start;
        ww_tracker_t projected = ww_tracker_project(&app->tracker, app->mode, phase_elapsed);
        ww_ui_render(&projected, ww_stable_clock_now(&app->clock));

        size_t len = ww_terminal_read_input(input, sizeof input, WW_REFRESH_INTERVAL_US);
        event = ww_input_parse(input, len);
    }

    ww_tracker_accumulate(&app->tracker, app->mode, ww_clock_monotonic_seconds() - phase_start);
    return event;
}

void ww_app_run(ww_app_t *app)
{
    ww_terminal_clear_screen();

    for (;;) {
        ww_input_event_t event = run_phase(app);
        if (event == WW_INPUT_QUIT) {
            break;
        }
        app->mode = ww_mode_next(app->mode);
    }

    /* Сводку печатаем уже в обычном режиме, чтобы работали переводы строк. */
    ww_terminal_restore();
    ww_ui_print_summary(&app->tracker);
}
