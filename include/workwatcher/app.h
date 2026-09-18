/**
 * @file app.h
 * @brief Логика приложения: цикл фаз «Работа»/«Дом».
 *
 * Модуль связывает трекер, часы, терминал и UI. Не разбирает аргументы
 * командной строки и не владеет терминалом — это ответственность main().
 */

#ifndef WORKWATCHER_APP_H
#define WORKWATCHER_APP_H

#include "workwatcher/clock.h"
#include "workwatcher/tracker.h"

/** Состояние приложения. */
typedef struct ww_app {
    ww_tracker_t tracker;    /**< Накопленное время завершённых фаз. */
    ww_stable_clock_t clock; /**< Стабильные настенные часы. */
    ww_mode_t mode;          /**< Активный режим. */
} ww_app_t;

/**
 * @brief Инициализирует состояние: обнуляет счётчики, фиксирует часы,
 *        выбирает начальный режим «Работа».
 * @param app Не NULL.
 */
void ww_app_init(ww_app_t *app);

/**
 * @brief Выполняет главный цикл до запроса на завершение.
 *
 * Предусловие: терминал уже переведён в raw-режим. После выхода из цикла
 * функция сама восстанавливает терминал и печатает итоговую сводку.
 *
 * @param app Инициализированное состояние; не NULL.
 */
void ww_app_run(ww_app_t *app);

#endif /* WORKWATCHER_APP_H */
