#include "workwatcher/terminal.h"

#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/*
 * Отключение отчётов мыши на случай аварийного выхода без endwin().
 * На обычном пути мышь включает/выключает ncurses (mousemask).
 */
#define MOUSE_DISABLE "\033[?1006l\033[?1003l\033[?1000l"

#define SIGNAL_EXIT_BASE 128

/* Состояние терминала; единственный экземпляр — см. комментарий в terminal.h. */
static struct termios g_saved_termios;
static volatile sig_atomic_t g_prepared = 0;

/* Пишет строку целиком в stdout, минуя stdio. Async-signal-safe. */
static void write_sequence(const char *seq, size_t len)
{
    while (len > 0) {
        ssize_t written = write(STDOUT_FILENO, seq, len);
        if (written <= 0) {
            return; /* терминал недоступен — дальнейшие попытки бессмысленны */
        }
        seq += written;
        len -= (size_t)written;
    }
}

void ww_terminal_restore(void)
{
    if (!g_prepared) {
        return;
    }
    /* Сначала просим терминал больше не слать события мыши, и лишь затем
       возвращаем канонические настройки ввода.

       Невычитанный ввод сбрасываем отдельно и применяем атрибуты с TCSANOW:
       TCSAFLUSH дополнительно ждал бы, пока терминал заберёт весь вывод, и
       мог бы заблокировать обработчик сигнала. */
    write_sequence(MOUSE_DISABLE, sizeof MOUSE_DISABLE - 1);
    tcflush(STDIN_FILENO, TCIFLUSH);
    tcsetattr(STDIN_FILENO, TCSANOW, &g_saved_termios);
    g_prepared = 0;
}

ww_status_t ww_terminal_prepare(void)
{
    if (g_prepared) {
        return WW_OK;
    }
    if (!isatty(STDIN_FILENO)) {
        return WW_ERR_NOT_A_TTY;
    }
    if (tcgetattr(STDIN_FILENO, &g_saved_termios) != 0) {
        return WW_ERR_TERMINAL_ATTR;
    }

    g_prepared = 1;
    return WW_OK;
}

static void handle_fatal_signal(int sig)
{
    ww_terminal_restore();
    _exit(SIGNAL_EXIT_BASE + sig);
}

void ww_terminal_install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handle_fatal_signal;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
}
