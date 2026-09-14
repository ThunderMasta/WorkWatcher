#ifndef UI_H
#define UI_H

#include "timer.h"

/* Рисует текущее состояние на экране (с учётом незавершённой фазы). */
void render(const TimeState *state, Mode mode, double phase_elapsed);

#endif /* UI_H */
