#include "workwatcher/input.h"

#include <stdbool.h>

#define ASCII_ESC 0x1BU

/* Биты поля Cb в отчётах мыши (общие для X10 и SGR). */
#define MOUSE_BUTTON_MASK        0x03U
#define MOUSE_BUTTON_RELEASE_X10 0x03U
#define MOUSE_FLAG_MOTION        0x20U
#define MOUSE_FLAG_WHEEL         0x40U

/* Длина X10-отчёта: ESC [ M Cb Cx Cy. */
#define X10_REPORT_LEN 6U

typedef struct mouse_report {
    size_t length;
    bool is_press;
    int col;
    int row;
} mouse_report_t;

static bool is_digit(unsigned char byte)
{
    return byte >= '0' && byte <= '9';
}

static bool is_button_press(unsigned long cb, bool released)
{
    return !released && (cb & MOUSE_FLAG_WHEEL) == 0 && (cb & MOUSE_FLAG_MOTION) == 0;
}

static bool rect_contains(const ww_rect_t *rect, int col, int row)
{
    if (rect == NULL || col <= 0 || row <= 0) {
        return false;
    }
    return col >= rect->col_min && col <= rect->col_max && row >= rect->row_min &&
           row <= rect->row_max;
}

static bool parse_sgr_mouse(const unsigned char *buf, size_t len, size_t pos, mouse_report_t *out)
{
    if (pos + 3 > len || buf[pos] != ASCII_ESC || buf[pos + 1] != '[' || buf[pos + 2] != '<') {
        return false;
    }

    size_t i = pos + 3;
    unsigned long cb = 0;
    bool have_cb = false;
    while (i < len && is_digit(buf[i])) {
        cb = cb * 10 + (unsigned long)(buf[i] - '0');
        have_cb = true;
        ++i;
    }
    if (!have_cb || i >= len || buf[i] != ';') {
        return false;
    }
    ++i;

    unsigned long cx = 0;
    bool have_cx = false;
    while (i < len && is_digit(buf[i])) {
        cx = cx * 10 + (unsigned long)(buf[i] - '0');
        have_cx = true;
        ++i;
    }
    if (!have_cx || i >= len || buf[i] != ';') {
        return false;
    }
    ++i;

    unsigned long cy = 0;
    bool have_cy = false;
    while (i < len && is_digit(buf[i])) {
        cy = cy * 10 + (unsigned long)(buf[i] - '0');
        have_cy = true;
        ++i;
    }
    if (!have_cy || i >= len || (buf[i] != 'M' && buf[i] != 'm')) {
        return false;
    }

    out->length = i + 1 - pos;
    out->is_press = is_button_press(cb, buf[i] == 'm');
    out->col = (int)cx;
    out->row = (int)cy;
    return true;
}

static bool parse_x10_mouse(const unsigned char *buf, size_t len, size_t pos, mouse_report_t *out)
{
    if (pos + X10_REPORT_LEN > len || buf[pos] != ASCII_ESC || buf[pos + 1] != '[' ||
        buf[pos + 2] != 'M') {
        return false;
    }

    unsigned long cb = (unsigned long)buf[pos + 3] - 32UL;
    bool released = (cb & MOUSE_BUTTON_MASK) == MOUSE_BUTTON_RELEASE_X10;

    out->length = X10_REPORT_LEN;
    out->is_press = is_button_press(cb, released);
    out->col = (int)buf[pos + 4] - 32;
    out->row = (int)buf[pos + 5] - 32;
    return true;
}

/* Префикс SGR/X10 без финального байта — ждём догрузки, не считаем клавишей. */
static bool is_incomplete_mouse(const unsigned char *buf, size_t len, size_t pos)
{
    if (pos >= len || buf[pos] != ASCII_ESC) {
        return false;
    }
    if (pos + 1 >= len) {
        return true; /* одиночный ESC в конце — тоже ждём хвост */
    }
    if (buf[pos + 1] != '[') {
        return false;
    }
    if (pos + 2 >= len) {
        return true;
    }

    if (buf[pos + 2] == '<') {
        size_t i = pos + 3;
        while (i < len && buf[i] != 'M' && buf[i] != 'm') {
            ++i;
        }
        return i >= len;
    }

    if (buf[pos + 2] == 'M') {
        return (len - pos) < X10_REPORT_LEN;
    }

    return false;
}

ww_input_event_t ww_input_parse(const unsigned char *buf, size_t len, const ww_rect_t *quit_rect,
                                ww_mouse_info_t *mouse_out, size_t *consumed)
{
    if (mouse_out != NULL) {
        mouse_out->present = 0;
        mouse_out->col = 0;
        mouse_out->row = 0;
    }
    if (consumed != NULL) {
        *consumed = len;
    }

    if (buf == NULL || len == 0) {
        if (consumed != NULL) {
            *consumed = 0;
        }
        return WW_INPUT_NONE;
    }

    bool toggle = false;
    bool quit = false;
    size_t i = 0;
    while (i < len) {
        mouse_report_t report;
        if (parse_sgr_mouse(buf, len, i, &report) || parse_x10_mouse(buf, len, i, &report)) {
            if (mouse_out != NULL) {
                mouse_out->present = 1;
                mouse_out->col = report.col;
                mouse_out->row = report.row;
            }
            if (report.is_press) {
                if (rect_contains(quit_rect, report.col, report.row)) {
                    quit = true;
                } else {
                    toggle = true;
                }
            }
            i += report.length;
            continue;
        }

        if (is_incomplete_mouse(buf, len, i)) {
            if (consumed != NULL) {
                *consumed = i;
            }
            break;
        }

        toggle = true;
        ++i;
    }

    if (consumed != NULL && i >= len) {
        *consumed = len;
    }

    if (quit) {
        return WW_INPUT_QUIT;
    }
    return toggle ? WW_INPUT_TOGGLE : WW_INPUT_NONE;
}
