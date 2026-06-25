#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>  // для функции sleep
#include <termios.h>  // для управления терминалом
#include <sys/select.h> // для select()

void print_time(double seconds_time) {
    // Вычисляем часы, минуты и секунды
    int hours = (int)(seconds_time / 3600);
    seconds_time -= hours * 3600;
    int minutes = (int)(seconds_time / 60);
    int seconds = (int)(seconds_time - minutes * 60);

    printf("%02d:%02d:%02d", hours, minutes, seconds);
}

// Функция для вывода разницы между двумя временами
void print_time_difference(
    double work_seconds,
    double home_seconds)
{
    printf("Работа: ");
    print_time(work_seconds);
    printf(" - Дом: ");
    print_time(home_seconds);
    printf("\r\n");
}

void print_finish_time(double work_seconds)
{
    int need_to_work = 8 * 3600 - work_seconds;

    if (need_to_work <= 0) {
        printf("Работа закончена!\r\n");

        return;
    }

    // Получаем текущее время
    time_t now = time(NULL);
    // Прибавляем оставшееся время до 8 часов
    time_t finish_time = now + (time_t)need_to_work;
    // Преобразуем в локальное время
    struct tm *tm_finish = localtime(&finish_time);

    // Выводим предполагаемое время окончания 8-часового рабочего дня
    printf(
        "Работа закончится: %02d:%02d:%02d\r\n",
        tm_finish->tm_hour,
        tm_finish->tm_min,
        tm_finish->tm_sec);
}

void print_times(
    double work_seconds,
    double home_seconds
)
{
    print_time_difference(work_seconds, home_seconds);
    print_finish_time(work_seconds);
}

void write_work(
    struct tm start_local_time,
    double work_seconds,
    double home_seconds)
{
    time_t t = time(NULL);

    time_t start_t = mktime(&start_local_time);

    double seconds_diff = difftime(t, start_t);

    print_times(seconds_diff + work_seconds, home_seconds);
}

void write_home(
    struct tm start_local_time,
    double work_seconds,
    double home_seconds)
{
    time_t t = time(NULL);

    time_t start_t = mktime(&start_local_time);

    double seconds_diff = difftime(t, start_t);

    print_times(work_seconds, seconds_diff + home_seconds);
}

// Функция для чтения символа с таймаутом в 0.01 секунду
// Возвращает символ, если он был введен, или -1 при таймауте
int read_char_with_timeout(void) {
    struct timeval tv;
    fd_set read_fds;
    int result;

    // Настройка таймаута в 0.01 секунду
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    // Инициализация набора дескрипторов
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    // Ожидание ввода
    result = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);

    if (result == -1) {
        // Ошибка
        return -1;
    } else if (result == 0) {
        // Таймаут
        return -1;
    } else {
        // Есть данные для чтения
        int ch = getchar();
        return ch;
    }
}

double get_work_seconds(
    double work_seconds_old,
    double home_seconds)
{
    time_t t = time(NULL);
    struct tm start_time = *localtime(&t);

    while (1)
    {
        system("clear");

        write_work(start_time, work_seconds_old, home_seconds);

        int ch = read_char_with_timeout();
        
        if (ch != -1) {
            // Если был введен символ, то выходим из цикла
            break;
        }
    }

    time_t t2 = time(NULL);

    double work_seconds = difftime(t2, t);

    return work_seconds + work_seconds_old;
}

double get_home_seconds(
    double work_seconds,
    double home_seconds_old)
{
    time_t t = time(NULL);
    struct tm start_time = *localtime(&t);

    while (1)
    {
        system("clear");

        write_home(start_time, work_seconds, home_seconds_old);

        int ch = read_char_with_timeout();
        
        if (ch != -1) {
            // Если был введен символ, то выходим из цикла
            break;
        }
    }

    time_t t2 = time(NULL);

    double home_seconds = difftime(t2, t);

    return home_seconds + home_seconds_old;
}

int main(void) {
    system("clear");

    system("/bin/stty raw"); // Set terminal to raw mode

    double work_seconds = 0;
    double home_seconds = 0;

    time_t t = time(NULL);
    struct tm start_time = *localtime(&t);

    while (1)
    {
        work_seconds = get_work_seconds(
            work_seconds,
            home_seconds
        );

        home_seconds = get_home_seconds(
            work_seconds,
            home_seconds
        );
    }

    system("/bin/stty cooked"); // Restore terminal to cooked mode

    return 0;
}
