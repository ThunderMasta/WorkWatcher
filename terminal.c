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

static struct termios g_saved_termios;
static bool g_terminal_is_raw = false;

void restore_terminal(void)
{
    if (g_terminal_is_raw) {
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

bool wait_for_keypress(void)
{
    if (stdin_readable(REFRESH_INTERVAL_US / 1000000,
                       REFRESH_INTERVAL_US % 1000000) <= 0) {
        return false; /* таймаут или прерывание */
    }

    /* Читаем напрямую из fd, а не через stdio getchar(): иначе getchar()
       сгребает несколько байт во внутренний буфер, и select() их больше
       не видит — часть нажатий «глатается».

       Считываем и отбрасываем ВЕСЬ ввод, пришедший одним нажатием, — как
       только что, так и добравшийся за KEY_TAIL_US. Так любая клавиша
       (печатная, стрелка, функциональная) даёт ровно одно переключение,
       и в буфере не остаётся "хвостов" для лишних срабатываний. */
    char buf[64];
    if (read(STDIN_FILENO, buf, sizeof buf) <= 0) {
        return false;
    }
    while (stdin_readable(0, KEY_TAIL_US) > 0) {
        if (read(STDIN_FILENO, buf, sizeof buf) <= 0) {
            break;
        }
    }
    return true;
}

void clear_screen(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}
