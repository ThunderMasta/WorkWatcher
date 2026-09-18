#include "workwatcher/input.h"

#include <stdbool.h>

#define ASCII_ESC 0x1BU

/* Клавиша выхода. Ctrl+Q доходит до приложения байтом XON только потому,
   что terminal.c отключает IXON. Cmd+Q использовать нельзя: его перехватывает
   само приложение терминала и в программу ничего не передаёт. */
#define CTRL_Q 0x11U

/* Биты поля Cb в отчётах мыши (общие для X10 и SGR). */
#define MOUSE_BUTTON_MASK        0x03U
#define MOUSE_BUTTON_RELEASE_X10 0x03U
#define MOUSE_FLAG_MOTION        0x20U
#define MOUSE_FLAG_WHEEL         0x40U

/* Длина X10-отчёта: ESC [ M Cb Cx Cy. */
#define X10_REPORT_LEN 6U

/* Результат разбора одного отчёта мыши. */
typedef struct mouse_report {
    size_t length; /* сколько байт занял отчёт */
    bool is_press; /* это нажатие кнопки (не отпускание/прокрутка/движение) */
} mouse_report_t;

static bool is_quit_byte(unsigned char byte)
{
    return byte == CTRL_Q;
}

static bool is_digit(unsigned char byte)
{
    return byte >= '0' && byte <= '9';
}

static bool is_button_press(unsigned long cb, bool released)
{
    return !released && (cb & MOUSE_FLAG_WHEEL) == 0 && (cb & MOUSE_FLAG_MOTION) == 0;
}

/*
 * Пытается разобрать SGR-отчёт мыши `ESC [ < Cb ; Cx ; Cy (M|m)` с позиции
 * pos. Возвращает true, если отчёт распознан полностью (тогда заполняет out).
 * Неполный или чужой ввод — false.
 */
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

    /* Пропускаем ; Cx ; Cy до финального байта. */
    while (i < len && buf[i] != 'M' && buf[i] != 'm') {
        ++i;
    }
    if (i >= len || !have_cb) {
        return false;
    }

    out->length = i + 1 - pos;
    out->is_press = is_button_press(cb, buf[i] == 'm');
    return true;
}

/*
 * Пытается разобрать X10-отчёт мыши `ESC [ M Cb Cx Cy` с позиции pos.
 * Используется терминалами без поддержки SGR-режима (1006).
 */
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
    return true;
}

ww_input_event_t ww_input_parse(const unsigned char *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return WW_INPUT_NONE;
    }

    bool toggle = false;
    size_t i = 0;
    while (i < len) {
        mouse_report_t report;
        if (parse_sgr_mouse(buf, len, i, &report) || parse_x10_mouse(buf, len, i, &report)) {
            toggle = toggle || report.is_press;
            i += report.length;
            continue;
        }

        if (is_quit_byte(buf[i])) {
            return WW_INPUT_QUIT;
        }

        /* Любой прочий байт — обычная клавиша. Продолжаем сканирование,
           чтобы не пропустить запрос на завершение в той же порции. */
        toggle = true;
        ++i;
    }

    return toggle ? WW_INPUT_TOGGLE : WW_INPUT_NONE;
}
