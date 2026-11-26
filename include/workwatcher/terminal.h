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

#endif /* WORKWATCHER_TERMINAL_H */
