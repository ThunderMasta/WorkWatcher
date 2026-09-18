/**
 * @file status.h
 * @brief Единые коды результата для функций, которые могут завершиться
 *        неуспешно.
 */

#ifndef WORKWATCHER_STATUS_H
#define WORKWATCHER_STATUS_H

/** Код результата операции. Ноль — успех, любое другое значение — ошибка. */
typedef enum ww_status {
    WW_OK = 0,              /**< Успех. */
    WW_ERR_NOT_A_TTY,       /**< stdin не является терминалом. */
    WW_ERR_TERMINAL_ATTR,   /**< Не удалось прочитать/установить атрибуты терминала. */
    WW_ERR_TERMINAL_IO,     /**< Ошибка записи управляющей последовательности в терминал. */
    WW_ERR_INVALID_ARGUMENT /**< Некорректный аргумент вызова. */
} ww_status_t;

/**
 * @brief Человекочитаемое описание кода результата.
 * @param status Код результата.
 * @return Статическая строка; никогда не NULL.
 */
const char *ww_status_describe(ww_status_t status);

#endif /* WORKWATCHER_STATUS_H */
