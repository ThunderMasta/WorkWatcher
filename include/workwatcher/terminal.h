/**
 * @file terminal.h
 * @brief Подготовка терминала и аварийное восстановление по сигналам.
 *
 * Режим ввода/вывода и мышь на обычном пути принадлежат ncurses (`ui`).
 * Этот модуль сохраняет исходные атрибуты до initscr() и умеет вернуть
 * их из обработчика сигнала, где endwin() вызывать нельзя.
 */

#ifndef WORKWATCHER_TERMINAL_H
#define WORKWATCHER_TERMINAL_H

#include "workwatcher/status.h"

/**
 * @brief Проверяет, что stdin — TTY, и запоминает исходные атрибуты.
 *
 * Вызывать до ww_ui_init(). Повторный вызов без restore безопасен.
 *
 * @return ::WW_OK либо код ошибки.
 */
ww_status_t ww_terminal_prepare(void);

/**
 * @brief Возвращает терминалу исходные настройки и отключает отчёты мыши.
 *
 * Идемпотентна и async-signal-safe: допустимо вызывать из обработчика
 * сигнала и через atexit(). Не вызывает endwin().
 */
void ww_terminal_restore(void);

/**
 * @brief Устанавливает обработчики SIGINT/SIGTERM/SIGHUP, которые
 *        восстанавливают терминал и завершают процесс.
 */
void ww_terminal_install_signal_handlers(void);

/**
 * @brief 1, если после выхода окно хоста будет скрыто (сводка там не видна).
 *
 * Сейчас: macOS + TERM_PROGRAM=Apple_Terminal.
 */
int ww_terminal_hides_host_window(void);

/**
 * @brief Очищает экран и scrollback (CSI), чтобы в скрытом окне не копился текст.
 *
 * Не из обработчика сигнала.
 */
void ww_terminal_clear_display(void);

/**
 * @brief Пытается скрыть окно хост-терминала после штатного выхода.
 *
 * На macOS при TERM_PROGRAM=Apple_Terminal — AppleScript
 * (`visible of front window`). Иначе no-op. Не из обработчика сигнала.
 */
void ww_terminal_hide_host_window(void);

#endif /* WORKWATCHER_TERMINAL_H */
