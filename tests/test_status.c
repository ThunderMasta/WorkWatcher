#include "workwatcher/status.h"

#include "suites.h"
#include "ww_test.h"

WW_TEST(every_status_has_description)
{
    const ww_status_t all[] = {WW_OK, WW_ERR_NOT_A_TTY, WW_ERR_TERMINAL_ATTR, WW_ERR_TERMINAL_IO,
                               WW_ERR_INVALID_ARGUMENT};
    for (size_t i = 0; i < sizeof all / sizeof all[0]; ++i) {
        const char *text = ww_status_describe(all[i]);
        WW_ASSERT(text != NULL);
        WW_ASSERT(text[0] != '\0');
    }
}

WW_TEST(unknown_status_is_safe)
{
    WW_ASSERT_EQ_STR(ww_status_describe((ww_status_t)9999), "неизвестная ошибка");
}

void suite_status(void)
{
    puts("status");
    WW_RUN(every_status_has_description);
    WW_RUN(unknown_status_is_safe);
}
