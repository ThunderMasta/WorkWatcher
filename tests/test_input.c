#include <string.h>

#include "workwatcher/input.h"

#include "suites.h"
#include "ww_test.h"

/* Разбирает строковый литерал (без завершающего нуля). */
static ww_input_event_t parse_str(const char *s)
{
    return ww_input_parse((const unsigned char *)s, strlen(s));
}

WW_TEST(empty_input_is_none)
{
    WW_ASSERT_EQ_INT(ww_input_parse(NULL, 0), WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(parse_str(""), WW_INPUT_NONE);
}

WW_TEST(plain_key_toggles)
{
    WW_ASSERT_EQ_INT(parse_str("a"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str(" "), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\r"), WW_INPUT_TOGGLE);
}

WW_TEST(escape_sequences_of_keys_toggle)
{
    WW_ASSERT_EQ_INT(parse_str("\033"), WW_INPUT_TOGGLE);     /* одиночный ESC */
    WW_ASSERT_EQ_INT(parse_str("\033[A"), WW_INPUT_TOGGLE);   /* стрелка вверх */
    WW_ASSERT_EQ_INT(parse_str("\033[15~"), WW_INPUT_TOGGLE); /* F5 */
    WW_ASSERT_EQ_INT(parse_str("\033OP"), WW_INPUT_TOGGLE);   /* F1 */
}

WW_TEST(ctrl_q_requests_quit)
{
    WW_ASSERT_EQ_INT(parse_str("\x11"), WW_INPUT_QUIT); /* Ctrl+Q */
}

WW_TEST(other_control_chars_toggle)
{
    /* Ctrl+C, Ctrl+D, Ctrl+\ — обычные клавиши: ISIG отключён, сигналов
       они не порождают. */
    WW_ASSERT_EQ_INT(parse_str("\x03"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\x04"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\x1c"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("q"), WW_INPUT_TOGGLE); /* без модификатора */
}

WW_TEST(quit_has_priority_over_toggle)
{
    /* Литералы разделены: "\x11a" парсился бы как один байт 0x11A. */
    WW_ASSERT_EQ_INT(parse_str("a\x11"), WW_INPUT_QUIT);
    WW_ASSERT_EQ_INT(parse_str("\x11"
                               "a"),
                     WW_INPUT_QUIT);
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5M\x11"), WW_INPUT_QUIT);
}

WW_TEST(sgr_mouse_press_toggles)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5M"), WW_INPUT_TOGGLE);  /* левая */
    WW_ASSERT_EQ_INT(parse_str("\033[<2;300;1M"), WW_INPUT_TOGGLE); /* правая, x > 223 */
}

WW_TEST(sgr_mouse_release_is_ignored)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5m"), WW_INPUT_NONE);
}

WW_TEST(sgr_mouse_wheel_and_motion_are_ignored)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<64;10;5M"), WW_INPUT_NONE); /* колесо вверх */
    WW_ASSERT_EQ_INT(parse_str("\033[<65;10;5M"), WW_INPUT_NONE); /* колесо вниз */
    WW_ASSERT_EQ_INT(parse_str("\033[<32;10;5M"), WW_INPUT_NONE); /* движение с кнопкой */
    WW_ASSERT_EQ_INT(parse_str("\033[<35;10;5M"), WW_INPUT_NONE); /* движение без кнопки */
}

WW_TEST(sgr_press_and_release_in_one_chunk_toggle_once)
{
    /* Одно физическое нажатие даёт press + release; результат — одно
       переключение. */
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5M\033[<0;10;5m"), WW_INPUT_TOGGLE);
}

WW_TEST(sgr_release_followed_by_key_toggles)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5mx"), WW_INPUT_TOGGLE);
}

WW_TEST(incomplete_sgr_sequence_is_treated_as_key)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\033[<;10;5M"), WW_INPUT_TOGGLE); /* нет Cb */
}

WW_TEST(x10_mouse_press_toggles_release_ignored)
{
    /* ESC [ M Cb Cx Cy, поля смещены на 32. */
    const unsigned char press[] = {0x1b, '[', 'M', 32 + 0, 32 + 10, 32 + 5};
    const unsigned char release[] = {0x1b, '[', 'M', 32 + 3, 32 + 10, 32 + 5};
    const unsigned char wheel[] = {0x1b, '[', 'M', 32 + 64, 32 + 10, 32 + 5};

    WW_ASSERT_EQ_INT(ww_input_parse(press, sizeof press), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(ww_input_parse(release, sizeof release), WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(ww_input_parse(wheel, sizeof wheel), WW_INPUT_NONE);
}

WW_TEST(incomplete_x10_sequence_is_treated_as_key)
{
    const unsigned char partial[] = {0x1b, '[', 'M', 32 + 0};
    WW_ASSERT_EQ_INT(ww_input_parse(partial, sizeof partial), WW_INPUT_TOGGLE);
}

void suite_input(void)
{
    puts("input");
    WW_RUN(empty_input_is_none);
    WW_RUN(plain_key_toggles);
    WW_RUN(escape_sequences_of_keys_toggle);
    WW_RUN(ctrl_q_requests_quit);
    WW_RUN(other_control_chars_toggle);
    WW_RUN(quit_has_priority_over_toggle);
    WW_RUN(sgr_mouse_press_toggles);
    WW_RUN(sgr_mouse_release_is_ignored);
    WW_RUN(sgr_mouse_wheel_and_motion_are_ignored);
    WW_RUN(sgr_press_and_release_in_one_chunk_toggle_once);
    WW_RUN(sgr_release_followed_by_key_toggles);
    WW_RUN(incomplete_sgr_sequence_is_treated_as_key);
    WW_RUN(x10_mouse_press_toggles_release_ignored);
    WW_RUN(incomplete_x10_sequence_is_treated_as_key);
}
