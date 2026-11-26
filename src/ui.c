#include "workwatcher/ui.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

#define CTRL_C 0x03

static int g_ui_active = 0;
static int g_colors = 0;
static WINDOW *g_frame = NULL;
static WINDOW *g_shadow = NULL;

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

static void destroy_windows(void)
{
    if (g_frame != NULL) {
        delwin(g_frame);
        g_frame = NULL;
    }
    if (g_shadow != NULL) {
        delwin(g_shadow);
        g_shadow = NULL;
    }
}

ww_status_t ww_ui_init(void)
{
    if (g_ui_active) {
        return WW_OK;
    }

    /* Локаль нужна для корректной ширины кириллицы в ncurses. */
    setlocale(LC_ALL, "");

    if (initscr() == NULL) {
        return WW_ERR_TERMINAL_IO;
    }

    /* raw: без ISIG, Ctrl+C приходит как символ 0x03, как раньше. */
    raw();
    noecho();
    nonl();
    curs_set(0);
    keypad(stdscr, TRUE);
    meta(stdscr, TRUE);
    leaveok(stdscr, TRUE);
    flushinp();

    /* Только нажатия кнопок; отпускание/колесо/движение отфильтруем ещё и
       в getmouse на случай расширенных отчётов терминала. */
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
    destroy_windows();
    /* Снимаем мышь до endwin, чтобы терминал не продолжал слать отчёты. */
    mousemask((mmask_t)0, NULL);
    endwin();
    g_ui_active = 0;
    g_colors = 0;
}

static void set_pair_win(WINDOW *win, int pair)
{
    if (g_colors) {
        wattrset(win, (int)COLOR_PAIR(pair));
    }
}

/* Пишет текст в pair, обрезая по экранным колонкам; остаток забивает пробелами. */
static void put_clipped_win(WINDOW *win, int y, int x, int width, int pair, const char *text)
{
    set_pair_win(win, pair);
    wmove(win, y, x);
    for (int i = 0; i < width; ++i) {
        waddch(win, ' ');
    }

    wmove(win, y, x);
    size_t cols = 0;
    mbstate_t st;
    memset(&st, 0, sizeof st);
    const char *s = text;

    while (*s != '\0' && cols < (size_t)width) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, s, (size_t)MB_CUR_MAX, &st);
        if (n == (size_t)-1 || n == (size_t)-2) {
            break;
        }
        if (n == 0) {
            break;
        }
        int w = wcwidth(wc);
        if (w < 0) {
            w = 1;
        }
        if (cols + (size_t)w > (size_t)width) {
            break;
        }
        waddnstr(win, s, (int)n);
        cols += (size_t)w;
        s += n;
    }
}

static void draw_title(WINDOW *win, int w, const char *title)
{
    int inner = w - 2;
    size_t title_cols = utf8_cols(title);
    if (title_cols > (size_t)inner) {
        title_cols = (size_t)inner;
    }
    int pad = (inner - (int)title_cols) / 2;

    set_pair_win(win, PAIR_TITLE);
    wmove(win, 0, 1);
    for (int i = 0; i < inner; ++i) {
        waddch(win, ' ');
    }
    put_clipped_win(win, 0, 1 + pad, inner - pad, PAIR_TITLE, title);
}

static void draw_hline_tee(WINDOW *win, int y, int w, int pair)
{
    set_pair_win(win, pair);
    mvwaddch(win, y, 0, ACS_LTEE);
    mvwaddch(win, y, w - 1, ACS_RTEE);
    mvwhline(win, y, 1, ACS_HLINE, w - 2);
}

static void draw_mode_line(WINDOW *win, int y, int x, const char *label, const char *value,
                           int active)
{
    int pair = active ? PAIR_ACTIVE : PAIR_IDLE;
    const char *mark = active ? ">" : " ";

    set_pair_win(win, pair);
    mvwaddstr(win, y, x, mark);
    mvwaddstr(win, y, x + 2, label);
    mvwaddstr(win, y, x + 10, value);
}

/* Создаёт или перемещает WINDOW рамки и тени под текущий размер терминала.
   Возвращает 0 при успехе, -1 если терминал слишком мал (рисуем fallback). */
static int ensure_windows(int max_y, int max_x)
{
    if (max_y < WIN_H + 1 || max_x < WIN_W + 1) {
        destroy_windows();
        return -1;
    }

    int top = (max_y - WIN_H) / 2;
    int left = (max_x - WIN_W) / 2;

    if (g_frame == NULL) {
        g_frame = newwin(WIN_H, WIN_W, top, left);
    } else {
        mvwin(g_frame, top, left);
        wresize(g_frame, WIN_H, WIN_W);
    }
    if (g_frame == NULL) {
        return -1;
    }

    int have_shadow = (top + WIN_H < max_y) && (left + WIN_W < max_x);
    if (have_shadow) {
        if (g_shadow == NULL) {
            g_shadow = newwin(WIN_H, WIN_W, top + 1, left + 1);
        } else {
            mvwin(g_shadow, top + 1, left + 1);
            wresize(g_shadow, WIN_H, WIN_W);
        }
        if (g_shadow == NULL) {
            destroy_windows();
            return -1;
        }
    } else if (g_shadow != NULL) {
        delwin(g_shadow);
        g_shadow = NULL;
    }

    return 0;
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

    if (g_colors) {
        wbkgd(stdscr, COLOR_PAIR(PAIR_DESKTOP) | (chtype)' ');
    }
    werase(stdscr);

    if (ensure_windows(max_y, max_x) != 0) {
        /* Узкий терминал: без рамки, прямо на stdscr. */
        put_clipped_win(stdscr, 0, 0, max_x > 0 ? max_x : 0, PAIR_WINDOW, title);
        if (max_y > 1) {
            put_clipped_win(stdscr, 1, 0, max_x, PAIR_ACTIVE, work_buf);
        }
        if (max_y > 2) {
            put_clipped_win(stdscr, 2, 0, max_x, PAIR_WINDOW, finish_buf);
        }
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }

    if (g_shadow != NULL) {
        if (g_colors) {
            wbkgd(g_shadow, COLOR_PAIR(PAIR_SHADOW) | ACS_CKBOARD);
        } else {
            wbkgd(g_shadow, ACS_CKBOARD);
        }
        werase(g_shadow);
    }

    if (g_colors) {
        wbkgd(g_frame, COLOR_PAIR(PAIR_WINDOW) | (chtype)' ');
    }
    werase(g_frame);
    set_pair_win(g_frame, PAIR_WINDOW);
    box(g_frame, 0, 0);

    draw_title(g_frame, WIN_W, title);
    draw_hline_tee(g_frame, 1, WIN_W, PAIR_WINDOW);

    draw_mode_line(g_frame, 2, 2, LABEL_WORK, work_buf, active == WW_MODE_WORK);
    draw_mode_line(g_frame, 3, 2, LABEL_HOME, home_buf, active == WW_MODE_HOME);
    put_clipped_win(g_frame, 5, 2, WIN_W - 4, PAIR_WINDOW, finish_buf);

    draw_hline_tee(g_frame, WIN_H - 2, WIN_W, PAIR_WINDOW);
    put_clipped_win(g_frame, WIN_H - 1, 1, WIN_W - 2, PAIR_STATUS, HINT_LINE);

    /* Углы нижней рамки поверх статусной полосы. */
    set_pair_win(g_frame, PAIR_WINDOW);
    mvwaddch(g_frame, WIN_H - 1, 0, ACS_LLCORNER);
    mvwaddch(g_frame, WIN_H - 1, WIN_W - 1, ACS_LRCORNER);

    /* Порядок: стол → тень → рамка (перекрытие даёт видимую тень справа/снизу). */
    wnoutrefresh(stdscr);
    if (g_shadow != NULL) {
        wnoutrefresh(g_shadow);
    }
    wnoutrefresh(g_frame);
    doupdate();
}

static ww_input_event_t event_from_mouse(void)
{
    MEVENT ev;
    if (getmouse(&ev) != OK) {
        return WW_INPUT_NONE;
    }

    const mmask_t press =
        (mmask_t)(BUTTON1_PRESSED | BUTTON2_PRESSED | BUTTON3_PRESSED);
    if ((ev.bstate & press) != 0U) {
        return WW_INPUT_TOGGLE;
    }
    return WW_INPUT_NONE;
}

ww_input_event_t ww_ui_poll_input(long timeout_us)
{
    if (!g_ui_active) {
        return WW_INPUT_NONE;
    }

    int ms = (int)(timeout_us / 1000L);
    if (ms < 0) {
        ms = 0;
    }
    timeout(ms);

    int ch = getch();
    if (ch == ERR) {
        return WW_INPUT_NONE;
    }
    if (ch == CTRL_C) {
        return WW_INPUT_QUIT;
    }
    if (ch == KEY_RESIZE) {
        return WW_INPUT_NONE;
    }
    if (ch == KEY_MOUSE) {
        return event_from_mouse();
    }

    /* Любая клавиша, включая KEY_* от keypad. */
    return WW_INPUT_TOGGLE;
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
