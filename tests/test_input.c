#include <string.h>

#include "workwatcher/input.h"

#include "suites.h"
#include "ww_test.h"

static ww_input_event_t parse_str(const char *s)
{
    return ww_input_parse((const unsigned char *)s, strlen(s), NULL, NULL, NULL);
}

static ww_input_event_t parse_str_quit(const char *s, const ww_rect_t *quit)
{
    return ww_input_parse((const unsigned char *)s, strlen(s), quit, NULL, NULL);
}

WW_TEST(empty_input_is_none)
{
    WW_ASSERT_EQ_INT(ww_input_parse(NULL, 0, NULL, NULL, NULL), WW_INPUT_NONE);
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
    WW_ASSERT_EQ_INT(parse_str("\033"), WW_INPUT_NONE); /* одиночный ESC — ждём хвост */
    WW_ASSERT_EQ_INT(parse_str("\033[A"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\033[15~"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\033OP"), WW_INPUT_TOGGLE);
}

WW_TEST(ctrl_c_toggles_not_quit)
{
    WW_ASSERT_EQ_INT(parse_str("\x03"), WW_INPUT_TOGGLE);
}

WW_TEST(other_control_chars_toggle)
{
    WW_ASSERT_EQ_INT(parse_str("\x04"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\x11"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\x1c"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("c"), WW_INPUT_TOGGLE);
}

WW_TEST(sgr_mouse_press_toggles)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5M"), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str("\033[<2;300;1M"), WW_INPUT_TOGGLE);
}

WW_TEST(sgr_mouse_release_is_ignored)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5m"), WW_INPUT_NONE);
}

WW_TEST(sgr_mouse_wheel_and_motion_are_ignored)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<64;10;5M"), WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(parse_str("\033[<65;10;5M"), WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(parse_str("\033[<32;10;5M"), WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(parse_str("\033[<35;10;5M"), WW_INPUT_NONE);
}

WW_TEST(sgr_motion_fills_mouse_info)
{
    const char *seq = "\033[<35;44;12M";
    ww_mouse_info_t mouse;
    WW_ASSERT_EQ_INT(ww_input_parse((const unsigned char *)seq, strlen(seq), NULL, &mouse, NULL),
                     WW_INPUT_NONE);
    WW_ASSERT(mouse.present);
    WW_ASSERT_EQ_INT(mouse.col, 44);
    WW_ASSERT_EQ_INT(mouse.row, 12);
}

WW_TEST(sgr_press_and_release_in_one_chunk_toggle_once)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5M\033[<0;10;5m"), WW_INPUT_TOGGLE);
}

WW_TEST(sgr_release_followed_by_key_toggles)
{
    WW_ASSERT_EQ_INT(parse_str("\033[<0;10;5mx"), WW_INPUT_TOGGLE);
}

WW_TEST(incomplete_sgr_waits_without_toggle)
{
    size_t consumed = 99;
    WW_ASSERT_EQ_INT(ww_input_parse((const unsigned char *)"\033[<", 3, NULL, NULL, &consumed),
                     WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(consumed, 0);

    consumed = 99;
    WW_ASSERT_EQ_INT(ww_input_parse((const unsigned char *)"\033[<0;10", 7, NULL, NULL, &consumed),
                     WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(consumed, 0);
}

WW_TEST(malformed_complete_sgr_treated_as_keys)
{
    /* Есть финальный M, но поля кривые — не incomplete, байты как клавиши. */
    WW_ASSERT_EQ_INT(parse_str("\033[<;10;5M"), WW_INPUT_TOGGLE);
}

WW_TEST(x10_mouse_press_toggles_release_ignored)
{
    const unsigned char press[] = {0x1b, '[', 'M', 32 + 0, 32 + 10, 32 + 5};
    const unsigned char release[] = {0x1b, '[', 'M', 32 + 3, 32 + 10, 32 + 5};
    const unsigned char wheel[] = {0x1b, '[', 'M', 32 + 64, 32 + 10, 32 + 5};

    WW_ASSERT_EQ_INT(ww_input_parse(press, sizeof press, NULL, NULL, NULL), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(ww_input_parse(release, sizeof release, NULL, NULL, NULL), WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(ww_input_parse(wheel, sizeof wheel, NULL, NULL, NULL), WW_INPUT_NONE);
}

WW_TEST(incomplete_x10_waits_without_toggle)
{
    const unsigned char partial[] = {0x1b, '[', 'M', 32 + 0};
    size_t consumed = 99;
    WW_ASSERT_EQ_INT(ww_input_parse(partial, sizeof partial, NULL, NULL, &consumed), WW_INPUT_NONE);
    WW_ASSERT_EQ_INT(consumed, 0);
}

WW_TEST(mouse_press_on_quit_rect_requests_quit)
{
    const ww_rect_t quit = {.col_min = 40, .col_max = 48, .row_min = 12, .row_max = 12};

    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;40;12M", &quit), WW_INPUT_QUIT);
    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;48;12M", &quit), WW_INPUT_QUIT);
    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;44;12M", &quit), WW_INPUT_QUIT);
}

WW_TEST(mouse_press_outside_quit_edges_toggles)
{
    const ww_rect_t quit = {.col_min = 40, .col_max = 48, .row_min = 12, .row_max = 12};

    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;39;12M", &quit), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;49;12M", &quit), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;44;11M", &quit), WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;44;13M", &quit), WW_INPUT_TOGGLE);
}

WW_TEST(quit_has_priority_over_toggle)
{
    const ww_rect_t quit = {.col_min = 40, .col_max = 48, .row_min = 12, .row_max = 12};

    WW_ASSERT_EQ_INT(parse_str_quit("\033[<0;10;5M\033[<0;44;12M", &quit), WW_INPUT_QUIT);
}

WW_TEST(x10_mouse_press_on_quit_rect_requests_quit)
{
    const ww_rect_t quit = {.col_min = 10, .col_max = 18, .row_min = 5, .row_max = 5};
    const unsigned char press[] = {0x1b, '[', 'M', 32 + 0, 32 + 12, 32 + 5};

    WW_ASSERT_EQ_INT(ww_input_parse(press, sizeof press, &quit, NULL, NULL), WW_INPUT_QUIT);
}

WW_TEST(key_then_incomplete_mouse_consumes_key_only)
{
    const char *seq = "a\033[<0;1";
    size_t consumed = 0;
    WW_ASSERT_EQ_INT(ww_input_parse((const unsigned char *)seq, strlen(seq), NULL, NULL, &consumed),
                     WW_INPUT_TOGGLE);
    WW_ASSERT_EQ_INT(consumed, 1);
}

void suite_input(void)
{
    puts("input");
    WW_RUN(empty_input_is_none);
    WW_RUN(plain_key_toggles);
    WW_RUN(escape_sequences_of_keys_toggle);
    WW_RUN(ctrl_c_toggles_not_quit);
    WW_RUN(other_control_chars_toggle);
    WW_RUN(sgr_mouse_press_toggles);
    WW_RUN(sgr_mouse_release_is_ignored);
    WW_RUN(sgr_mouse_wheel_and_motion_are_ignored);
    WW_RUN(sgr_motion_fills_mouse_info);
    WW_RUN(sgr_press_and_release_in_one_chunk_toggle_once);
    WW_RUN(sgr_release_followed_by_key_toggles);
    WW_RUN(incomplete_sgr_waits_without_toggle);
    WW_RUN(malformed_complete_sgr_treated_as_keys);
    WW_RUN(x10_mouse_press_toggles_release_ignored);
    WW_RUN(incomplete_x10_waits_without_toggle);
    WW_RUN(mouse_press_on_quit_rect_requests_quit);
    WW_RUN(mouse_press_outside_quit_edges_toggles);
    WW_RUN(quit_has_priority_over_toggle);
    WW_RUN(x10_mouse_press_on_quit_rect_requests_quit);
    WW_RUN(key_then_incomplete_mouse_consumes_key_only);
}
