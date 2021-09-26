/*
 * gcc -Wall -o retype retype2.c -lutf8proc -lunistring $(ncursesw5-config --cflags --libs)
 * indent -kr -ts4 -nut -l80 *.c
 * apt install libncursesw5-dev libunistring-dev libutf8proc-dev
 */

#include <stdio.h>
#include <stdint.h>
#include <locale.h>
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include <inttypes.h>
#include <wchar.h>

#include <unitypes.h>
#include <uniconv.h>
#include <unistdio.h>
#include <unistr.h>
#include <uniwidth.h>
#include <utf8proc.h>
#include <fcntl.h>
#include <unistd.h>

#define GREY_PAIR    5
#define GOOD_PAIR    6
#define ERROR_PAIR   7

#define XCH_UNKNOWN   0
#define XCH_BACKSPACE 3
#define XCH_CHAR      4
#define XCH_ALT       5
#define XCH_CTRL      7
#define XCH_FUNCTION  6
#define XCH_SPECIAL   8

#define XCH_KEY_ESC       1
#define XCH_KEY_NEWLINE   2
#define XCH_KEY_BACKSPACE 3

#define MAX_LEN 81
#define MAX_LINES 99999

FILE *logger;

WINDOW *pad;

struct xchar {
    uint8_t type;
    wint_t ch;
};

wint_t getwchar_()
{
    // printf("getwchar_(): ");
    wint_t ch = getwchar();
    // printf("%d \r\n", ch);
    return ch;
}

struct xchar getxchar()
{
    wint_t ch = getwchar_();

    if (ch == 13) {
        return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_NEWLINE };
    }

    if (ch == 127) {
        return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_BACKSPACE };
    }

    if (ch < 27) {
        return (struct xchar) {.type = XCH_CTRL,.ch = ch + 96 };
    }

    if (ch == 27) {

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        ch = getwchar_();
        if (ch == 79) {
            fcntl(STDIN_FILENO, F_SETFL, flags);
            return (struct xchar) {.type = XCH_FUNCTION,.ch = getwchar_() };
        }
        if (ch == EOF) {
            fcntl(STDIN_FILENO, F_SETFL, flags);
            return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_ESC };
        }
        
        fcntl(STDIN_FILENO, F_SETFL, flags);
        return (struct xchar) {.type = XCH_ALT,.ch = ch };
    }
    return (struct xchar) {.type = XCH_CHAR,.ch = ch };
}

struct xchar getxchar_() {
    struct xchar xch = getxchar();

    if (1) return xch;
    //////////////////
    
    switch (xch.type) {
    case XCH_UNKNOWN:
        printf("Unknown: %d \r\n", xch.ch);
        break;
    case XCH_CHAR:
        printf("Normal char: %lc (%d)\r\n", xch.ch, xch.ch);
        break;

    case XCH_CTRL:
        printf("Ctrl+%c \r\n", xch.ch);
        break;
    case XCH_FUNCTION:
        printf("Function: %d \r\n", xch.ch);
        break;
    case XCH_ALT:
        printf("Alt+%c \r\n", xch.ch);
        break;
    case XCH_SPECIAL:
        switch (xch.ch)
        {
        case XCH_KEY_ESC:
            printf("<Escape> \r\n");
            break;
        case XCH_KEY_NEWLINE:
            printf("<Enter> \r\n");
            break;

        case XCH_KEY_BACKSPACE:
            printf("<Backspace> \r\n");
            break;
        
        default:
            printf("Special: %d \r\n", xch.ch);
        }
        break;
    }

    return xch;
}

void overtype_last_line()
{
    do {
        struct xchar xch = getxchar_();
        fseek(stdin, 0, SEEK_END);
    } while (true);
}

void print_previous_lines()
{
}

struct winsize get_winsize()
{
    struct winsize winsz;
    ioctl(0, TIOCGWINSZ, &winsz);
    return winsz;
}

void run_in_available_screen()
{
    struct winsize winsz = get_winsize();
    fprintf(logger, "running in window size: %d rows / %d columns\n",
            winsz.ws_row, winsz.ws_col);


    int lines_total = 10;
    pad = newpad(lines_total, winsz.ws_col);

    print_previous_lines();
    overtype_last_line();
}

static void sig_handler(int sig)
{
    if (SIGWINCH == sig) {
        run_in_available_screen();
    }
}

static void init_colors() {
    start_color();
    init_pair(GREY_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(ERROR_PAIR, COLOR_RED + 8, COLOR_YELLOW + 8);
    init_pair(GOOD_PAIR, COLOR_GREEN + 8, COLOR_BLACK);
    clear();
}

int main(void)
{
    logger = fopen("retype.log", "a+");

    signal(SIGWINCH, sig_handler);

    freopen("/dev/tty", "rw", stdin);
    setlocale(LC_ALL, "");
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    init_colors();

    run_in_available_screen();

    getch();
    endwin();
    exit(0);
}
