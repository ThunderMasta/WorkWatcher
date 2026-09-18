#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "suites.h"
#include "ww_test.h"

int ww_test_total = 0;
int ww_test_failed = 0;
int ww_test_current_failed = 0;

int main(void)
{
    /* Фиксируем часовой пояс: тесты форматирования времени не должны
       зависеть от окружения. */
    setenv("TZ", "UTC", 1);
    tzset();

    suite_status();
    suite_clock();
    suite_tracker();
    suite_input();
    suite_ui();

    printf("\n%d тестов, %d провалено\n", ww_test_total, ww_test_failed);
    return ww_test_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
