#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/select.h>

static struct termios saved_termios;
static int terminal_is_raw = 0;

static void restore_terminal(void)
{
    if (terminal_is_raw) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
        terminal_is_raw = 0;
    }
}

static int setup_terminal(void)
{
    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return -1;
    }

    struct termios raw = saved_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return -1;
    }

    terminal_is_raw = 1;
    return 0;
}

static void on_signal(int sig)
{
    restore_terminal();
    _exit(128 + sig);
}

#define REFRESH_INTERVAL_US 250000

static void clear_screen(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

void print_time(double seconds_time) {
    // Вычисляем часы, минуты и секунды
    int hours = (int)(seconds_time / 3600);
    seconds_time -= hours * 3600;
    int minutes = (int)(seconds_time / 60);
    int seconds = (int)(seconds_time - minutes * 60);

    printf("%02d:%02d:%02d", hours, minutes, seconds);
}

// Функция для вывода разницы между двумя временами
void print_time_difference(
    double work_seconds,
    double home_seconds)
{
    printf("Работа: ");
    print_time(work_seconds);
    printf(" - Дом: ");
    print_time(home_seconds);
}

static void print_finish_line(double work_seconds)
{
    int need_to_work = 8 * 3600 - work_seconds;

    if (need_to_work <= 0) {
        printf("Работа закончена!");
        return;
    }

    time_t now = time(NULL);
    time_t finish_time = now + (time_t)need_to_work;
    struct tm *tm_finish = localtime(&finish_time);

    printf(
        "Работа закончится: %02d:%02d:%02d",
        tm_finish->tm_hour,
        tm_finish->tm_min,
        tm_finish->tm_sec);
}

static void refresh_times(double work_seconds, double home_seconds)
{
    printf("\033[1;1H\033[2K");
    print_time_difference(work_seconds, home_seconds);

    printf("\033[2;1H\033[2K");
    print_finish_line(work_seconds);

    fflush(stdout);
}

typedef enum {
    MODE_WORK,
    MODE_HOME
} Mode;

static void refresh_display(
    Mode mode,
    time_t phase_start,
    double work_accumulated,
    double home_accumulated)
{
    double elapsed = difftime(time(NULL), phase_start);
    double work_seconds = work_accumulated;
    double home_seconds = home_accumulated;

    if (mode == MODE_WORK) {
        work_seconds += elapsed;
    } else {
        home_seconds += elapsed;
    }

    refresh_times(work_seconds, home_seconds);
}

typedef struct {
    double work_seconds;
    double home_seconds;
} TimeState;

// Функция для чтения символа с таймаутом
// Возвращает символ, если он был введен, или -1 при таймауте
static int read_char_with_timeout(void) {
    struct timeval tv;
    fd_set read_fds;
    int result;

    tv.tv_sec = REFRESH_INTERVAL_US / 1000000;
    tv.tv_usec = REFRESH_INTERVAL_US % 1000000;

    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    result = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);

    if (result == -1) {
        return -1;
    } else if (result == 0) {
        return -1;
    } else {
        return getchar();
    }
}

static TimeState run_phase(Mode mode, TimeState state)
{
    time_t phase_start = time(NULL);

    while (1) {
        refresh_display(
            mode,
            phase_start,
            state.work_seconds,
            state.home_seconds);

        if (read_char_with_timeout() != -1) {
            break;
        }
    }

    double elapsed = difftime(time(NULL), phase_start);

    if (mode == MODE_WORK) {
        state.work_seconds += elapsed;
    } else {
        state.home_seconds += elapsed;
    }

    return state;
}

int main(void) {
    clear_screen();

    if (setup_terminal() != 0) {
        fprintf(stderr, "Не удалось настроить терминал\n");
        return 1;
    }

    atexit(restore_terminal);

    struct sigaction sa;
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    TimeState state = {0, 0};

    while (1)
    {
        state = run_phase(MODE_WORK, state);
        state = run_phase(MODE_HOME, state);
    }

    restore_terminal();

    return 0;
}
