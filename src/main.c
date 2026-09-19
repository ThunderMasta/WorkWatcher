/**
 * @file main.c
 * @brief Точка входа: разбор аргументов, подготовка терминала, запуск.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "workwatcher/app.h"
#include "workwatcher/status.h"
#include "workwatcher/terminal.h"
#include "workwatcher/version.h"

static void print_usage(FILE *out)
{
    fputs("Использование: workwatcher [--help] [--version]\n"
          "\n"
          "Консольный учёт времени в режимах «Работа» и «Дом».\n"
          "\n"
          "Управление:\n"
          "  любая клавиша / кнопка мыши  сменить режим\n"
          "  Ctrl+C                        завершить работу\n"
          "\n"
          "Опции:\n"
          "  -h, --help     показать эту справку и выйти\n"
          "  -V, --version  показать версию и выйти\n",
          out);
}

static void print_version(void)
{
    printf("%s %s\n", WW_APP_NAME, WW_VERSION_STRING);
}

/* Возвращает код завершения, если аргументы требуют немедленного выхода,
   иначе -1. */
static int handle_arguments(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(stdout);
            return EXIT_SUCCESS;
        }
        if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            print_version();
            return EXIT_SUCCESS;
        }
        fprintf(stderr, "Неизвестный аргумент: %s\n\n", arg);
        print_usage(stderr);
        return EXIT_FAILURE;
    }
    return -1;
}

int main(int argc, char **argv)
{
    int early_exit = handle_arguments(argc, argv);
    if (early_exit >= 0) {
        return early_exit;
    }

    ww_status_t status = ww_terminal_enter_raw_mode();
    if (status != WW_OK) {
        fprintf(stderr, "Не удалось настроить терминал: %s\n", ww_status_describe(status));
        return EXIT_FAILURE;
    }
    atexit(ww_terminal_restore);
    ww_terminal_install_signal_handlers();

    ww_app_t app;
    ww_app_init(&app);
    ww_app_run(&app);

    return EXIT_SUCCESS;
}
