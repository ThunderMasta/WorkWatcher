#include "terminal.h"

#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

/* Как часто перерисовывать экран, когда не нажато ни одной клавиши. */
#define REFRESH_INTERVAL_US 250000

/* Сколько ждать "хвост" одного нажатия: остаток многобайтовой
   последовательности (стрелки, функциональные клавиши) и событие
   отпускания клавиши. */
#define KEY_TAIL_US 40000

/* Включаем отчёты о нажатиях/отпусканиях кнопок мыши: 1000 — сами
   события, 1006 — SGR-кодировка (координаты не обрезаются до 223). */
#define MOUSE_ENABLE  "\033[?1000h\033[?1006h"
#define MOUSE_DISABLE "\033[?1006l\033[?1000l"

static struct termios g_saved_termios;
static bool g_terminal_is_raw = false;

void restore_terminal(void)
{
    if (g_terminal_is_raw) {
        /* Сначала просим терминал больше не слать события мыши, и лишь
           затем возвращаем канонические настройки ввода. */
        if (write(STDOUT_FILENO, MOUSE_DISABLE, sizeof MOUSE_DISABLE - 1) < 0) {
            /* Игнорируем: терминал всё равно восстанавливаем. */
        }
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_termios);
        g_terminal_is_raw = false;
    }
}

int setup_terminal(void)
{
    if (tcgetattr(STDIN_FILENO, &g_saved_termios) != 0) {
        return -1;
    }

    struct termios raw = g_saved_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return -1;
    }

    /* Просим терминал присылать события кнопок мыши. */
    if (write(STDOUT_FILENO, MOUSE_ENABLE, sizeof MOUSE_ENABLE - 1) < 0) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_termios);
        return -1;
    }

    g_terminal_is_raw = true;
    return 0;
}

static void handle_signal(int sig)
{
    restore_terminal();
    _exit(128 + sig);
}

void install_signal_handlers(void)
{
    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Ждёт готовности stdin к чтению не дольше заданного времени.
   Возвращает >0 если есть данные, 0 при таймауте, <0 при ошибке. */
static int stdin_readable(long sec, long usec)
{
    struct timeval tv = { .tv_sec = sec, .tv_usec = usec };
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    return select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);
}

/* Решает по накопленному вводу, нужно ли переключить режим.

   Клавиатура и нажатие кнопки мыши — да. Отпускание кнопки, прокрутка
   и движение мыши — нет: иначе одно физическое нажатие давало бы два
   переключения (на press и на release). */
static bool input_requests_toggle(const char *buf, size_t len)
{
    size_t i = 0;
    while (i < len) {
        /* SGR-последовательность мыши: ESC [ < Cb ; Cx ; Cy (M|m). */
        if ((unsigned char)buf[i] == 0x1b && i + 2 < len &&
            buf[i + 1] == '[' && buf[i + 2] == '<') {
            size_t j      = i + 3;
            long   cb     = 0;
            bool   have_cb = false;
            while (j < len && buf[j] >= '0' && buf[j] <= '9') {
                cb = cb * 10 + (buf[j] - '0');
                have_cb = true;
                j++;
            }
            /* Пропускаем ; Cx ; Cy до финального байта M или m. */
            while (j < len && buf[j] != 'M' && buf[j] != 'm') {
                j++;
            }
            if (j < len) {
                bool is_wheel  = (cb & 64) != 0;
                bool is_motion = (cb & 32) != 0;
                if (have_cb && buf[j] == 'M' && !is_wheel && !is_motion) {
                    return true; /* нажатие кнопки мыши */
                }
                i = j + 1; /* не переключаем: пропускаем событие */
                continue;
            }
            /* Неполная последовательность — считаем обычным вводом. */
            return true;
        }

        /* Любой прочий байт — обычная клавиша (в т.ч. ESC, стрелки). */
        return true;
    }
    return false;
}

bool wait_for_keypress(void)
{
    if (stdin_readable(REFRESH_INTERVAL_US / 1000000,
                       REFRESH_INTERVAL_US % 1000000) <= 0) {
        return false; /* таймаут или прерывание */
    }

    /* Читаем напрямую из fd, а не через stdio getchar(): иначе getchar()
       сгребает несколько байт во внутренний буфер, и select() их больше
       не видит — часть нажатий «глатается».

       Собираем ВЕСЬ ввод, пришедший одним нажатием, — как только что, так
       и добравшийся за KEY_TAIL_US, — чтобы затем целиком его разобрать
       и отбросить "хвосты", не порождающие лишних переключений. */
    char   buf[256];
    size_t len = 0;

    ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
    if (n <= 0) {
        return false;
    }
    len = (size_t)n;

    while (len < sizeof buf && stdin_readable(0, KEY_TAIL_US) > 0) {
        n = read(STDIN_FILENO, buf + len, sizeof buf - len);
        if (n <= 0) {
            break;
        }
        len += (size_t)n;
    }

    return input_requests_toggle(buf, len);
}

void clear_screen(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}
