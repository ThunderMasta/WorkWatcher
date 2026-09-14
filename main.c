#include <stdio.h>
#include <stdlib.h>

#include "terminal.h"
#include "timer.h"
#include "ui.h"

/* Одна фаза: обновляем экран, пока не нажата клавиша,
   затем засчитываем накопленное время в нужный счётчик. */
static void run_phase(Mode mode, TimeState *state)
{
    double phase_start = monotonic_seconds();

    for (;;) {
        render(state, mode, monotonic_seconds() - phase_start);
        if (wait_for_keypress()) {
            break;
        }
    }

    accumulate(state, mode, monotonic_seconds() - phase_start);
}

int main(void)
{
    if (setup_terminal() != 0) {
        fprintf(stderr, "Не удалось настроить терминал\n");
        return 1;
    }

    atexit(restore_terminal);
    install_signal_handlers();
    clear_screen();

    TimeState state = {0};
    for (;;) {
        run_phase(MODE_WORK, &state);
        run_phase(MODE_HOME, &state);
    }
}
