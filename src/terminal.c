#include "workwatcher/terminal.h"

#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "workwatcher/config.h"

#define MICROSECONDS_PER_SECOND 1000000L

/*
 * Отчёты о нажатиях/отпусканиях кнопок мыши: 1000 — сами события,
 * 1006 — SGR-кодировка (координаты не обрезаются до 223).
 */
#define MOUSE_ENABLE  "\033[?1000h\033[?1006h"
#define MOUSE_DISABLE "\033[?1006l\033[?1000l"

#define CLEAR_SCREEN "\033[2J\033[H"

#define SIGNAL_EXIT_BASE 128

/* Состояние терминала; единственный экземпляр — см. комментарий в terminal.h. */
static struct termios g_saved_termios;
static volatile sig_atomic_t g_raw_mode_active = 0;

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
    if (!g_raw_mode_active) {
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
    g_raw_mode_active = 0;
}

ww_status_t ww_terminal_enter_raw_mode(void)
{
    if (g_raw_mode_active) {
        return WW_OK;
    }
    if (!isatty(STDIN_FILENO)) {
        return WW_ERR_NOT_A_TTY;
    }
    if (tcgetattr(STDIN_FILENO, &g_saved_termios) != 0) {
        return WW_ERR_TERMINAL_ATTR;
    }

    struct termios raw = g_saved_termios;
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t) ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return WW_ERR_TERMINAL_ATTR;
    }

    if (write(STDOUT_FILENO, MOUSE_ENABLE, sizeof MOUSE_ENABLE - 1) < 0) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_termios);
        return WW_ERR_TERMINAL_IO;
    }

    g_raw_mode_active = 1;
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

/* Ждёт готовности stdin не дольше timeout_us.
   Возвращает >0 если есть данные, 0 при таймауте, <0 при ошибке. */
static int wait_stdin_readable(long timeout_us)
{
    struct timeval tv = {
        .tv_sec = timeout_us / MICROSECONDS_PER_SECOND,
        .tv_usec = (suseconds_t)(timeout_us % MICROSECONDS_PER_SECOND),
    };
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    return select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);
}

size_t ww_terminal_read_input(unsigned char *buf, size_t capacity, long timeout_us)
{
    if (buf == NULL || capacity == 0) {
        return 0;
    }
    if (wait_stdin_readable(timeout_us) <= 0) {
        return 0;
    }

    /* Читаем напрямую из дескриптора, а не через stdio: иначе getchar()
       сгребает несколько байт во внутренний буфер, и select() их больше не
       видит — часть нажатий «глотается». */
    ssize_t n = read(STDIN_FILENO, buf, capacity);
    if (n <= 0) {
        return 0;
    }
    size_t len = (size_t)n;

    /* Добираем «хвост» того же нажатия, чтобы вызывающая сторона получила
       последовательность целиком и смогла отбросить лишние события. */
    while (len < capacity && wait_stdin_readable(WW_KEY_TAIL_US) > 0) {
        n = read(STDIN_FILENO, buf + len, capacity - len);
        if (n <= 0) {
            break;
        }
        len += (size_t)n;
    }

    return len;
}

void ww_terminal_clear_screen(void)
{
    write_sequence(CLEAR_SCREEN, sizeof CLEAR_SCREEN - 1);
}
