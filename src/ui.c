#include "workwatcher/ui.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#if defined(__has_include)
#if __has_include(<ncursesw/ncurses.h>)
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif
#else
#include <ncurses.h>
#endif

#include "workwatcher/config.h"
#include "workwatcher/version.h"

#define LABEL_WORK "Работа"
#define LABEL_HOME "Дом"

#define HINT_LINE "Клавиша/мышь — сменить режим, Ctrl+C — выход"

/* Геометрия окна, включая рамку.
 * 0 заголовок, 1 разделитель, 2–5 контент, 6 разделитель, 7 статус. */
#define WIN_W 52
#define WIN_H 8

#define PAIR_DESKTOP 1
#define PAIR_WINDOW  2
#define PAIR_TITLE   3
#define PAIR_ACTIVE  4
#define PAIR_IDLE    5
#define PAIR_STATUS  6
#define PAIR_SHADOW  7

static int g_ui_active = 0;
static int g_colors = 0;

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

/* Число экранных колонок UTF-8 строки в текущей локали. */
static size_t utf8_cols(const char *s)
{
    size_t cols = 0;
    mbstate_t st;
    memset(&st, 0, sizeof st);

    while (*s != '\0') {
        wchar_t wc;
        size_t n = mbrtowc(&wc, s, (size_t)MB_CUR_MAX, &st);
        if (n == (size_t)-1 || n == (size_t)-2) {
            ++cols;
            ++s;
            memset(&st, 0, sizeof st);
            continue;
        }
        if (n == 0) {
            break;
        }
        int w = wcwidth(wc);
        cols += (w > 0) ? (size_t)w : 1U;
        s += n;
    }
    return cols;
}

static void setup_colors(void)
{
    g_colors = 0;
    if (!has_colors()) {
        return;
    }
    if (start_color() == ERR) {
        return;
    }
    g_colors = 1;

    /* Классическая DOS/Turbo Vision палитра: синий рабочий стол, жёлтый
       акцент активного режима, бирюзовая строка статуса. */
    init_pair(PAIR_DESKTOP, COLOR_CYAN, COLOR_BLUE);
    init_pair(PAIR_WINDOW, COLOR_WHITE, COLOR_BLUE);
    init_pair(PAIR_TITLE, COLOR_BLACK, COLOR_CYAN);
    init_pair(PAIR_ACTIVE, COLOR_YELLOW, COLOR_BLUE);
    init_pair(PAIR_IDLE, COLOR_CYAN, COLOR_BLUE);
    init_pair(PAIR_STATUS, COLOR_BLACK, COLOR_CYAN);
    init_pair(PAIR_SHADOW, COLOR_BLACK, COLOR_BLACK);
}

ww_status_t ww_ui_init(void)
{
    if (g_ui_active) {
        return WW_OK;
    }

    setlocale(LC_ALL, "");

    if (initscr() == NULL) {
        return WW_ERR_TERMINAL_IO;
    }

    /* raw: Ctrl+C → байт 0x03. Ввод — select/read + ww_input_parse (не getch). */
    raw();
    noecho();
    nonl();
    curs_set(0);
    keypad(stdscr, FALSE);
    leaveok(stdscr, FALSE);
    flushinp();

    /* Мышь через ncurses output; байты забирает наш read. */
    mousemask((mmask_t)(BUTTON1_PRESSED | BUTTON2_PRESSED | BUTTON3_PRESSED), NULL);
    mouseinterval(0);

    setup_colors();

    g_ui_active = 1;
    return WW_OK;
}

void ww_ui_shutdown(void)
{
    if (!g_ui_active) {
        return;
    }
    mousemask((mmask_t)0, NULL);
    endwin();
    g_ui_active = 0;
    g_colors = 0;
}

static void set_pair(int pair)
{
    if (g_colors) {
        attrset((int)COLOR_PAIR(pair));
    }
}

static void fill_rect(int y, int x, int h, int w, int pair, chtype ch)
{
    set_pair(pair);
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            mvaddch(y + row, x + col, ch);
        }
    }
}

static void draw_box(int top, int left, int h, int w, int pair)
{
    set_pair(pair);

    mvaddch(top, left, ACS_ULCORNER);
    mvaddch(top, left + w - 1, ACS_URCORNER);
    mvaddch(top + h - 1, left, ACS_LLCORNER);
    mvaddch(top + h - 1, left + w - 1, ACS_LRCORNER);

    for (int col = 1; col < w - 1; ++col) {
        mvaddch(top, left + col, ACS_HLINE);
        mvaddch(top + h - 1, left + col, ACS_HLINE);
    }
    for (int row = 1; row < h - 1; ++row) {
        mvaddch(top + row, left, ACS_VLINE);
        mvaddch(top + row, left + w - 1, ACS_VLINE);
    }
}

static void draw_hline_tee(int y, int left, int w, int pair)
{
    set_pair(pair);
    mvaddch(y, left, ACS_LTEE);
    mvaddch(y, left + w - 1, ACS_RTEE);
    for (int col = 1; col < w - 1; ++col) {
        mvaddch(y, left + col, ACS_HLINE);
    }
}

static void put_clipped(int y, int x, int width, int pair, const char *text)
{
    set_pair(pair);
    move(y, x);
    for (int i = 0; i < width; ++i) {
        addch(' ');
    }

    move(y, x);
    size_t cols = 0;
    mbstate_t st;
    memset(&st, 0, sizeof st);
    const char *s = text;

    while (*s != '\0' && cols < (size_t)width) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, s, (size_t)MB_CUR_MAX, &st);
        if (n == (size_t)-1 || n == (size_t)-2 || n == 0) {
            break;
        }
        int w = wcwidth(wc);
        if (w < 0) {
            w = 1;
        }
        if (cols + (size_t)w > (size_t)width) {
            break;
        }
        addnstr(s, (int)n);
        cols += (size_t)w;
        s += n;
    }
}

static void draw_title(int top, int left, int w, const char *title)
{
    int inner = w - 2;
    size_t title_cols = utf8_cols(title);
    if (title_cols > (size_t)inner) {
        title_cols = (size_t)inner;
    }
    int pad = (inner - (int)title_cols) / 2;

    set_pair(PAIR_TITLE);
    move(top, left + 1);
    for (int i = 0; i < inner; ++i) {
        addch(' ');
    }
    put_clipped(top, left + 1 + pad, inner - pad, PAIR_TITLE, title);
}

static void draw_mode_line(int y, int x, const char *label, const char *value, int active)
{
    int pair = active ? PAIR_ACTIVE : PAIR_IDLE;
    const char *mark = active ? ">" : " ";

    set_pair(pair);
    mvaddstr(y, x, mark);
    mvaddstr(y, x + 2, label);
    mvaddstr(y, x + 10, value);
}

void ww_ui_render(const ww_tracker_t *projected, ww_mode_t active, double now_realtime)
{
    char work_buf[WW_UI_DURATION_BUFSIZE];
    char home_buf[WW_UI_DURATION_BUFSIZE];
    char finish_buf[WW_UI_FINISH_BUFSIZE];
    char title[64];

    if (!g_ui_active || projected == NULL) {
        return;
    }

    double work = ww_tracker_elapsed(projected, WW_MODE_WORK);
    double home = ww_tracker_elapsed(projected, WW_MODE_HOME);

    ww_ui_format_duration(work, work_buf, sizeof work_buf);
    ww_ui_format_duration(home, home_buf, sizeof home_buf);
    ww_ui_format_finish_line(work, now_realtime, finish_buf, sizeof finish_buf);

    const char *mode_label = (active == WW_MODE_HOME) ? LABEL_HOME : LABEL_WORK;
    snprintf(title, sizeof title, " %s - %s ", WW_APP_NAME, mode_label);

    int max_y = 0;
    int max_x = 0;
    getmaxyx(stdscr, max_y, max_x);

    erase();

    if (g_colors) {
        bkgdset(COLOR_PAIR(PAIR_DESKTOP) | (chtype)' ');
        bkgd(COLOR_PAIR(PAIR_DESKTOP) | (chtype)' ');
    }

    if (max_y < WIN_H + 1 || max_x < WIN_W + 1) {
        put_clipped(0, 0, max_x > 0 ? max_x : 0, PAIR_WINDOW, title);
        if (max_y > 1) {
            put_clipped(1, 0, max_x, PAIR_ACTIVE, work_buf);
        }
        if (max_y > 2) {
            put_clipped(2, 0, max_x, PAIR_WINDOW, finish_buf);
        }
        refresh();
        return;
    }

    int top = (max_y - WIN_H) / 2;
    int left = (max_x - WIN_W) / 2;

    if (top + WIN_H < max_y && left + WIN_W < max_x) {
        fill_rect(top + 1, left + WIN_W, WIN_H, 1, PAIR_SHADOW, ACS_CKBOARD);
        fill_rect(top + WIN_H, left + 1, 1, WIN_W, PAIR_SHADOW, ACS_CKBOARD);
    }

    fill_rect(top, left, WIN_H, WIN_W, PAIR_WINDOW, ' ');
    draw_box(top, left, WIN_H, WIN_W, PAIR_WINDOW);
    draw_title(top, left, WIN_W, title);
    draw_hline_tee(top + 1, left, WIN_W, PAIR_WINDOW);

    int cx = left + 2;
    draw_mode_line(top + 2, cx, LABEL_WORK, work_buf, active == WW_MODE_WORK);
    draw_mode_line(top + 3, cx, LABEL_HOME, home_buf, active == WW_MODE_HOME);
    put_clipped(top + 5, cx, WIN_W - 4, PAIR_WINDOW, finish_buf);

    draw_hline_tee(top + WIN_H - 2, left, WIN_W, PAIR_WINDOW);
    put_clipped(top + WIN_H - 1, left + 1, WIN_W - 2, PAIR_STATUS, HINT_LINE);

    set_pair(PAIR_WINDOW);
    mvaddch(top + WIN_H - 1, left, ACS_LLCORNER);
    mvaddch(top + WIN_H - 1, left + WIN_W - 1, ACS_LRCORNER);

    /* Каждый кадр — полная отрисовка: иначе после первого refresh ncurses
       может не вытолкнуть смену цифр таймера в физический терминал. */
    clearok(stdscr, TRUE);
    refresh();
}

static int wait_stdin(long timeout_us)
{
    if (timeout_us < 0L) {
        timeout_us = 0L;
    }
    struct timeval tv = {
        .tv_sec = timeout_us / 1000000L,
        .tv_usec = (suseconds_t)(timeout_us % 1000000L),
    };
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    return select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);
}

ww_input_event_t ww_ui_poll_input(long timeout_us)
{
    unsigned char buf[WW_INPUT_BUFFER_SIZE];

    if (!g_ui_active) {
        return WW_INPUT_NONE;
    }

    if (wait_stdin(timeout_us) <= 0) {
        return WW_INPUT_NONE;
    }

    ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
    if (n <= 0) {
        return WW_INPUT_NONE;
    }
    size_t len = (size_t)n;

    while (len < sizeof buf && wait_stdin(WW_KEY_TAIL_US) > 0) {
        n = read(STDIN_FILENO, buf + len, sizeof buf - len);
        if (n <= 0) {
            break;
        }
        len += (size_t)n;
    }

    return ww_input_parse(buf, len);
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
