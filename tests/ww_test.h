/**
 * @file ww_test.h
 * @brief Минимальный каркас юнит-тестов без внешних зависимостей.
 *
 * Использование:
 *
 *     WW_TEST(my_case) { WW_ASSERT_EQ_INT(2 + 2, 4); }
 *
 *     void my_suite(void) { WW_RUN(my_case); }
 *
 * Каждый набор (suite) объявляется в tests/suites.h и вызывается из
 * tests/test_main.c. Сбой утверждения прерывает текущий тест, но не
 * остальные; итоговый код возврата ненулевой, если хоть один тест упал.
 */

#ifndef WW_TEST_H
#define WW_TEST_H

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Счётчики; определены в test_main.c. */
extern int ww_test_total;
extern int ww_test_failed;

/* Флаг сбоя текущего теста; выставляется WW_FAIL(). */
extern int ww_test_current_failed;

#define WW_TEST(name) static void test_##name(void)

#define WW_RUN(name)                                                                               \
    do {                                                                                           \
        ww_test_current_failed = 0;                                                                \
        ++ww_test_total;                                                                           \
        test_##name();                                                                             \
        if (ww_test_current_failed) {                                                              \
            ++ww_test_failed;                                                                      \
            printf("  FAIL  %s\n", #name);                                                         \
        } else {                                                                                   \
            printf("  ok    %s\n", #name);                                                         \
        }                                                                                          \
    } while (0)

#define WW_FAIL(fmt, ...)                                                                          \
    do {                                                                                           \
        ww_test_current_failed = 1;                                                                \
        fprintf(stderr, "%s:%d: " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);                      \
        return;                                                                                    \
    } while (0)

#define WW_ASSERT(cond)                                                                            \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            WW_FAIL("утверждение ложно: %s", #cond);                                               \
        }                                                                                          \
    } while (0)

#define WW_ASSERT_EQ_INT(actual, expected)                                                         \
    do {                                                                                           \
        long long a_ = (long long)(actual);                                                        \
        long long e_ = (long long)(expected);                                                      \
        if (a_ != e_) {                                                                            \
            WW_FAIL("%s == %lld, ожидалось %lld", #actual, a_, e_);                                \
        }                                                                                          \
    } while (0)

#define WW_ASSERT_EQ_STR(actual, expected)                                                         \
    do {                                                                                           \
        const char *a_ = (actual);                                                                 \
        const char *e_ = (expected);                                                               \
        if (strcmp(a_, e_) != 0) {                                                                 \
            WW_FAIL("%s == \"%s\", ожидалось \"%s\"", #actual, a_, e_);                            \
        }                                                                                          \
    } while (0)

#define WW_ASSERT_NEAR(actual, expected, eps)                                                      \
    do {                                                                                           \
        double a_ = (actual);                                                                      \
        double e_ = (expected);                                                                    \
        if (fabs(a_ - e_) > (eps)) {                                                               \
            WW_FAIL("%s == %.9g, ожидалось %.9g (±%g)", #actual, a_, e_, (double)(eps));           \
        }                                                                                          \
    } while (0)

#endif /* WW_TEST_H */
