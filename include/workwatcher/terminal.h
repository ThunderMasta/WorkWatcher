/**
 * @file terminal.h
 * @brief Низкоуровневая работа с терминалом: raw-режим, отчёты мыши,
 *        чтение ввода с таймаутом, аварийное восстановление по сигналам.
 *
 * Модуль хранит единственный экземпляр состояния терминала (исходные
 * атрибуты), поскольку восстанавливать его требуется из обработчика
 * сигнала, где произвольный контекст недоступен.
 */

#ifndef WORKWATCHER_TERMINAL_H
#define WORKWATCHER_TERMINAL_H

#include <stddef.h>

#include "workwatcher/status.h"

/**
 * @brief Переводит терминал в raw-режим и включает отчёты о кнопках мыши.
 *
 * Отключаются эхо, канонический ввод и генерация сигналов клавишами,
 * поэтому Ctrl+C приходит приложению как обычный байт. Повторный вызов без
 * промежуточного ww_terminal_restore() безопасен и ничего не делает.
 *
 * @return ::WW_OK либо код ошибки.
 */
ww_status_t ww_terminal_enter_raw_mode(void);

/**
 * @brief Возвращает терминалу исходные настройки.
 *
 * Идемпотентна и async-signal-safe: допустимо вызывать из обработчика
 * сигнала и через atexit().
 */
void ww_terminal_restore(void);

/**
 * @brief Устанавливает обработчики SIGINT/SIGTERM/SIGHUP, которые
 *        восстанавливают терминал и завершают процесс.
 */
void ww_terminal_install_signal_handlers(void);

/**
 * @brief Ждёт ввод не дольше @p timeout_us и читает всё, что пришло
 *        «одним нажатием» (включая хвост многобайтовой последовательности).
 *
 * @param buf        Буфер назначения; не NULL.
 * @param capacity   Размер буфера.
 * @param timeout_us Максимальное ожидание первого байта, микросекунды.
 * @return Число прочитанных байт; 0 при таймауте, ошибке или прерывании.
 */
size_t ww_terminal_read_input(unsigned char *buf, size_t capacity, long timeout_us);

/**
 * @brief Очищает экран и переводит курсор в левый верхний угол.
 */
void ww_terminal_clear_screen(void);

#endif /* WORKWATCHER_TERMINAL_H */
