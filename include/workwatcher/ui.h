/**
 * @file ui.h
 * @brief Форматирование и отрисовка кадра в стиле DOS через ncurses.
 *
 * Функции форматирования чистые (не обращаются к часам и терминалу) и
 * покрыты юнит-тестами; ww_ui_render() собирает их результат и рисует
 * окно. Ввод по-прежнему читается модулем terminal/input — ncurses
 * используется только для вывода.
 */

#ifndef WORKWATCHER_UI_H
#define WORKWATCHER_UI_H

#include <stddef.h>

#include "workwatcher/status.h"
#include "workwatcher/tracker.h"

/** Достаточный размер буфера для ww_ui_format_duration(). */
#define WW_UI_DURATION_BUFSIZE 32U

/** Достаточный размер буфера для ww_ui_format_finish_line(). */
#define WW_UI_FINISH_BUFSIZE 96U

/**
 * @brief Форматирует длительность как `ЧЧ:ММ:СС`.
 *
 * Дробная часть отбрасывается; отрицательные значения считаются нулём.
 *
 * @param seconds Длительность.
 * @param buf     Буфер назначения; не NULL.
 * @param size    Размер буфера.
 * @return Длина результата без завершающего нуля (как у snprintf).
 */
size_t ww_ui_format_duration(double seconds, char *buf, size_t size);

/**
 * @brief Формирует строку о времени окончания рабочего дня.
 *
 * Если рабочая норма уже выполнена — «Работа закончена!», иначе —
 * «Работа закончится: ЧЧ:ММ:СС» в локальном часовом поясе.
 *
 * Момент окончания считается как одно непрерывное значение
 * (@p now_realtime + остаток) и округляется к целым секундам один раз —
 * два независимых округления давали бы дрожание на ±1 секунду.
 *
 * @param work_seconds Отработано секунд (включая текущую фазу).
 * @param now_realtime Текущие настенные часы, UNIX-секунды с долями.
 * @param buf          Буфер назначения; не NULL.
 * @param size         Размер буфера.
 * @return Длина результата без завершающего нуля.
 */
size_t ww_ui_format_finish_line(double work_seconds, double now_realtime, char *buf, size_t size);

/**
 * @brief Инициализирует ncurses и цветовую схему DOS.
 *
 * Вызывать после ww_terminal_enter_raw_mode(); затем —
 * ww_terminal_reassert(), потому что initscr() может сбросить termios
 * и отчёты мыши. Идемпотентна.
 *
 * @return ::WW_OK либо код ошибки.
 */
ww_status_t ww_ui_init(void);

/**
 * @brief Завершает работу ncurses (endwin).
 *
 * Идемпотентна; допустимо вызывать из atexit() и перед печатью сводки.
 */
void ww_ui_shutdown(void);

/**
 * @brief Отрисовывает DOS-окно поверх предыдущего кадра.
 *
 * @param projected    Состояние с учётом незавершённой фазы,
 *                     см. ww_tracker_project().
 * @param active       Активный режим (подсвечивается в окне).
 * @param now_realtime Текущие настенные часы, UNIX-секунды с долями.
 */
void ww_ui_render(const ww_tracker_t *projected, ww_mode_t active, double now_realtime);

/**
 * @brief Печатает итоговую сводку в обычном (не raw) режиме терминала.
 *
 * Вызывать после ww_ui_shutdown() и ww_terminal_restore().
 */
void ww_ui_print_summary(const ww_tracker_t *tracker);

#endif /* WORKWATCHER_UI_H */
