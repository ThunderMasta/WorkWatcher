/**
 * @file input.h
 * @brief События ввода и байтовый разбор (для тестов / протокола).
 *
 * Рабочий путь: ww_ui_poll_input() читает stdin и вызывает ww_input_parse().
 * Парсер чистый (SGR/X10) и полностью покрыт юнит-тестами.
 */

#ifndef WORKWATCHER_INPUT_H
#define WORKWATCHER_INPUT_H

#include <stddef.h>

/** Событие, на которое должно отреагировать приложение. */
typedef enum ww_input_event {
    WW_INPUT_NONE = 0, /**< Реакции не требуется (отпускание кнопки мыши,
                            прокрутка, движение мыши, пустой ввод). */
    WW_INPUT_TOGGLE, /**< Нажата клавиша или кнопка мыши: сменить режим. */
    WW_INPUT_QUIT    /**< Запрошено завершение (Ctrl+C). */
} ww_input_event_t;

/**
 * @brief Разбирает порцию ввода, пришедшую за одно нажатие.
 *
 * Правила:
 *  - управляющий символ Ctrl+C (0x03) даёт ::WW_INPUT_QUIT и имеет
 *    приоритет над остальным вводом (в raw-режиме ISIG отключён, поэтому
 *    драйвер терминала не превращает его в SIGINT);
 *  - нажатие кнопки мыши (SGR `ESC [ < Cb ; Cx ; Cy M` или X10
 *    `ESC [ M Cb Cx Cy`) даёт ::WW_INPUT_TOGGLE;
 *  - отпускание кнопки, прокрутка и движение мыши игнорируются, иначе
 *    одно физическое нажатие давало бы два переключения;
 *  - любой другой байт (в т. ч. ESC-последовательности клавиш) считается
 *    обычной клавишей и даёт ::WW_INPUT_TOGGLE;
 *  - пустой ввод даёт ::WW_INPUT_NONE.
 *
 * @param buf Байты ввода; может быть NULL, если @p len равен 0.
 * @param len Число байт.
 */
ww_input_event_t ww_input_parse(const unsigned char *buf, size_t len);

#endif /* WORKWATCHER_INPUT_H */
