/**
 * @file config.h
 * @brief Параметры приложения, задаваемые на этапе компиляции.
 *
 * Все настраиваемые константы собраны в одном месте. Каждую можно
 * переопределить флагом компилятора, например:
 *
 *     make CPPFLAGS+='-DWW_WORKDAY_SECONDS=27000'
 */

#ifndef WORKWATCHER_CONFIG_H
#define WORKWATCHER_CONFIG_H

/** Секунд в минуте. */
#define WW_SECONDS_PER_MINUTE 60L

/** Секунд в часе. */
#define WW_SECONDS_PER_HOUR (60L * WW_SECONDS_PER_MINUTE)

/** Длительность рабочего дня, до которой считается время окончания. */
#ifndef WW_WORKDAY_SECONDS
#define WW_WORKDAY_SECONDS (8L * WW_SECONDS_PER_HOUR)
#endif

/** Период перерисовки экрана при отсутствии ввода, микросекунды. */
#ifndef WW_REFRESH_INTERVAL_US
#define WW_REFRESH_INTERVAL_US 250000L
#endif

/**
 * Ожидание «хвоста» одного нажатия, микросекунды: остаток многобайтовой
 * последовательности (стрелки, функциональные клавиши, отчёты мыши) и
 * событие отпускания клавиши.
 */
#ifndef WW_KEY_TAIL_US
#define WW_KEY_TAIL_US 40000L
#endif

/** Размер буфера для одной порции ввода с терминала, байты. */
#ifndef WW_INPUT_BUFFER_SIZE
#define WW_INPUT_BUFFER_SIZE 256U
#endif

#endif /* WORKWATCHER_CONFIG_H */
