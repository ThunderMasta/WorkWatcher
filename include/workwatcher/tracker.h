/**
 * @file tracker.h
 * @brief Учёт времени по режимам «Работа» и «Дом».
 *
 * Модуль не зависит от источников времени и терминала: он оперирует только
 * переданными ему длительностями, что делает его полностью тестируемым.
 */

#ifndef WORKWATCHER_TRACKER_H
#define WORKWATCHER_TRACKER_H

/** Режим (фаза), в котором сейчас идёт учёт времени. */
typedef enum ww_mode {
    WW_MODE_WORK = 0, /**< Рабочее время. */
    WW_MODE_HOME,     /**< Личное время. */
    WW_MODE_COUNT     /**< Число режимов; не является режимом. */
} ww_mode_t;

/** Накопленное время по каждому режиму, секунды с долями. */
typedef struct ww_tracker {
    double elapsed[WW_MODE_COUNT];
} ww_tracker_t;

/**
 * @brief Обнуляет счётчики.
 * @param tracker Не NULL.
 */
void ww_tracker_init(ww_tracker_t *tracker);

/**
 * @brief Прибавляет длительность к счётчику указанного режима.
 * @param tracker Не NULL.
 * @param mode    Режим; значения вне диапазона игнорируются.
 * @param seconds Длительность; отрицательные значения игнорируются.
 */
void ww_tracker_accumulate(ww_tracker_t *tracker, ww_mode_t mode, double seconds);

/**
 * @brief Накопленное время указанного режима.
 * @return Секунды; 0.0 для режима вне диапазона.
 */
double ww_tracker_elapsed(const ww_tracker_t *tracker, ww_mode_t mode);

/**
 * @brief Возвращает копию трекера с учётом незавершённой фазы.
 *
 * Используется для отрисовки: накопленные значения не меняются, а к
 * активному режиму прибавляется время текущей фазы.
 *
 * @param tracker       Исходное состояние; не NULL.
 * @param active        Активный режим.
 * @param phase_seconds Сколько уже длится текущая фаза.
 */
ww_tracker_t ww_tracker_project(const ww_tracker_t *tracker, ww_mode_t active,
                                double phase_seconds);

/**
 * @brief Следующий режим в цикле «Работа → Дом → Работа …».
 */
ww_mode_t ww_mode_next(ww_mode_t mode);

/**
 * @brief Проверяет, что значение является корректным режимом.
 */
int ww_mode_is_valid(ww_mode_t mode);

#endif /* WORKWATCHER_TRACKER_H */
