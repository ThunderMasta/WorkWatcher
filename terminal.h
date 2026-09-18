#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>

/* Переводит stdin в raw-режим. Возвращает 0 при успехе, -1 при ошибке. */
int setup_terminal(void);

/* Восстанавливает исходные настройки терминала (идемпотентно). */
void restore_terminal(void);

/* Ставит обработчики SIGINT/SIGTERM, восстанавливающие терминал. */
void install_signal_handlers(void);

/* Ждёт не дольше интервала обновления нажатия клавиши или кнопки мыши.
   Возвращает true, только если было именно нажатие (не отпускание,
   не прокрутка и не движение мыши). */
bool wait_for_keypress(void);

/* Очищает экран и ставит курсор в начало. */
void clear_screen(void);

#endif /* TERMINAL_H */
