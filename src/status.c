#include "workwatcher/status.h"

const char *ww_status_describe(ww_status_t status)
{
    switch (status) {
    case WW_OK:
        return "успех";
    case WW_ERR_NOT_A_TTY:
        return "стандартный ввод не является терминалом";
    case WW_ERR_TERMINAL_ATTR:
        return "не удалось изменить атрибуты терминала";
    case WW_ERR_TERMINAL_IO:
        return "ошибка записи в терминал";
    case WW_ERR_INVALID_ARGUMENT:
        return "некорректный аргумент";
    }
    return "неизвестная ошибка";
}
