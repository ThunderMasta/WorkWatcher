#ifndef TIMER_H
#define TIMER_H

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR   (60 * SECONDS_PER_MINUTE)

/* Длительность рабочего дня, до которой считается время окончания. */
#define WORKDAY_SECONDS (8 * SECONDS_PER_HOUR)

typedef enum { MODE_WORK, MODE_HOME } Mode;

typedef struct {
    double work_seconds;
    double home_seconds;
} TimeState;

/* Монотонное время в секундах (устойчиво к переводу системных часов). */
double monotonic_seconds(void);

/* Настенные часы в секундах, с долями (для времени окончания). */
double realtime_seconds(void);

/* Настенные часы, устойчивые к ручному переводу системного времени:
   после первой точки отсчёта идут от монотонных часов, поэтому скачок
   системных часов не смещает вычисленное время окончания. */
double stable_realtime_seconds(void);

/* Добавляет elapsed к счётчику, соответствующему текущему режиму. */
void accumulate(TimeState *state, Mode mode, double elapsed);

#endif /* TIMER_H */
