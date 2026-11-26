/**
 * @file input.h
 * @brief События ввода и байтовый разбор (SGR/X10 мышь, клавиши).
 *
 * Рабочий путь: ww_ui_poll_input() читает stdin и вызывает ww_input_parse()
 * с прямоугольником кнопки «Выход». Парсер чистый и покрыт юнит-тестами.
 */

#ifndef WORKWATCHER_INPUT_H
#define WORKWATCHER_INPUT_H

#include <stddef.h>

/** Событие, на которое должно отреагировать приложение. */
typedef enum ww_input_event {
    WW_INPUT_NONE = 0, /**< Реакции не требуется (отпускание кнопки мыши,
                            прокрутка, движение мыши, пустой ввод). */
    WW_INPUT_TOGGLE, /**< Нажата клавиша или мышь вне «Выход»: сменить режим. */
    WW_INPUT_QUIT    /**< Клик по кнопке «Выход». */
} ww_input_event_t;

/**
 * Прямоугольник на экране в координатах отчёта мыши (1-based, включительно).
 * Используется для попадания в кнопку «Выход».
 */
typedef struct ww_rect {
    int col_min;
    int col_max;
    int row_min;
    int row_max;
} ww_rect_t;

/**
 * @brief Разбирает порцию ввода, пришедшую за одно нажатие.
 *
 * Правила:
 *  - нажатие мыши внутри @p quit_rect (если не NULL) → ::WW_INPUT_QUIT;
 *  - нажатие мыши вне кнопки → ::WW_INPUT_TOGGLE;
 *  - отпускание, прокрутка и движение мыши игнорируются;
 *  - любой другой байт (клавиша, в т. ч. Ctrl+C) → ::WW_INPUT_TOGGLE;
 *  - пустой ввод → ::WW_INPUT_NONE.
 *  QUIT имеет приоритет над TOGGLE в одной порции.
 *
 * @param buf       Байты ввода; может быть NULL, если @p len равен 0.
 * @param len       Число байт.
 * @param quit_rect Зона кнопки «Выход» или NULL (тогда мышь не даёт QUIT).
 */
ww_input_event_t ww_input_parse(const unsigned char *buf, size_t len, const ww_rect_t *quit_rect);

#endif /* WORKWATCHER_INPUT_H */
