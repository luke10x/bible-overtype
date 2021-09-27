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
#define XCH_KEY_RESIZE    4

#define MAX_LEN 81
#define MAX_LINES 99999

FILE *logger;

WINDOW *pad;

char original_lines[MAX_LINES][MAX_LEN];
char broken_lines[MAX_LINES][MAX_LEN];

int original_lines_total;

struct xchar {
    uint8_t type;
    wint_t ch;
    char * str;
};

static volatile int resized = 1;

void print_grey(const int row, const int col, const char *line)
{
    wmove(pad, row, col);
    attron(COLOR_PAIR(GREY_PAIR));
    waddstr(pad, line);

    attroff(COLOR_PAIR(GREY_PAIR));
}

static void sig_handler(int sig)
{
  signal(SIGWINCH, SIG_IGN);

    // signal(SIGWINCH, sig_handler);
    if (SIGWINCH == sig) {
        // run_in_available_screen();
        resized = 1;
    }
    signal(SIGWINCH, sig_handler);
}

char * combine_bytes(int n, uint8_t *bytes) {
    char *result = malloc(3);

    result[0] = bytes[0];
    result[1] = bytes[1];
    result[2] = 0;

    return result;
}

/**
 * Values to test it with: ė
 */
uint8_t pending_char[4] = {0 ,0 ,0, 0};
int pending_curr = 0;
struct xchar getxchar()
{
    uint8_t ch;
    do {
        if (resized) {
            resized = 0;
            // printf("resize detected \r\n");
            return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_RESIZE };

        }

        halfdelay (10);
        ch = getch();

        if (ch == 255) {
            continue;
        }
        // printf("4 debug %d\r\n", ch);

        if (ch == 255) ch = getch(); // FIXME where is this coming from? If ever..

        int seventh = (ch >> 7) & 1;
        int sixth = (ch >> 6) & 1;

        if (seventh && sixth) {
            pending_char[0] = ch;
            pending_char[1] = 0;
            pending_char[2] = 0;
            pending_char[3] = 0;
            pending_curr = 1;

            printf("zeroth byte %d \r\n", ch);
            int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
            do {
                uint8_t ch = getch();

                int seventh = (ch >> 7) & 1;
                int sixth = (ch >> 6) & 1;
                if (seventh && !sixth) {
                    printf("byte nr %d %d\r\n", pending_curr, ch);

                    pending_char[pending_curr] = ch;
                    pending_curr++;
                } else {
                    fcntl(STDIN_FILENO, F_SETFL, flags);
                    // fseek(stdin, 0, SEEK_END);
                    // freopen("/dev/tty", "rw", stdin);
                    getch(); // FIXME more reliable way to clean it perhaps???

                    return (struct xchar) {
                        .type = XCH_CHAR,
                        .ch = '@',
                        .str = combine_bytes(pending_curr, pending_char)
                    };
                };
            }
            while (pending_curr < 4);
            endwin();
            perror("Too many bytes in unicode sequence");
            exit(1);
        }

        if (ch == 10) {
            return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_NEWLINE };
        }

        if (ch == 7 || ch == 8) {
            return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_BACKSPACE };
        }

        if (ch < 27) {
            return (struct xchar) {.type = XCH_CTRL,.ch = ch + 96 };
        }

        if (ch == 27) {
            int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

            ch = getch();

            printf("what %d \r\n", ch);
            if (ch == 255) {
                fcntl(STDIN_FILENO, F_SETFL, flags);
                return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_ESC };
            }

            fcntl(STDIN_FILENO, F_SETFL, flags);
            return (struct xchar) {.type = XCH_ALT,.ch = ch };
        }
        return (struct xchar) {.type = XCH_CHAR,.ch = ch };
    }
    while (ch == ERR || ch == KEY_RESIZE);
}

struct xchar getxchar_()
{
    struct xchar xch = getxchar();

    if (1)
        return xch;
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
        switch (xch.ch) {
        case XCH_KEY_ESC:
            printf("<Escape> \r\n");
            break;
        case XCH_KEY_NEWLINE:
            printf("<Enter> \r\n");
            break;
        case XCH_KEY_BACKSPACE:
            printf("<Backspace> \r\n");
            break;
        case XCH_KEY_RESIZE:
            printf("<Resize> \r\n");
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

        switch (xch.type) {
        case XCH_CHAR:
            printf("%lc", xch.ch, xch.str);
            refresh();
            break;
        case XCH_SPECIAL:
            switch (xch.ch) {
            case XCH_KEY_ESC:
                printf("<Escape> \r\n");
                break;
            case XCH_KEY_NEWLINE:
                printf("<Enter> \r\n");
                break;
            case XCH_KEY_BACKSPACE:
                printf("<Backspace> \r\n");
                break;
            case XCH_KEY_RESIZE:
                fit_in_available_screen();
                break;

            default:
                printf("Special: %d \r\n", xch.ch);
            }
            break;
        }
        
        fseek(stdin, 0, SEEK_END);
    } while (true);
}

void print_previous_lines(int number_of_lines)
{
    for(int i = 0; i < number_of_lines; i++) {
        printf("%s\r\n", broken_lines[i]);
    }
}

struct winsize get_winsize()
{
    struct winsize winsz;
    ioctl(0, TIOCGWINSZ, &winsz);
    return winsz;
}

uint8_t normalize(const uint32_t c)
{
    uint8_t *output;

    char *input_ = malloc(sizeof(const uint32_t) + 1);
    sprintf(input_, "%lc", c);

    const int first_ch_size = strlen(input_);   //u8_mblen(&c, sizeof(uint32_t));

    utf8proc_map((unsigned char *) input_, first_ch_size, &output,
                 UTF8PROC_DECOMPOSE | UTF8PROC_NULLTERM | UTF8PROC_STABLE |
                 UTF8PROC_STRIPMARK | UTF8PROC_CASEFOLD);

    return *output;
}

uint8_t *skip_n_unicode_chars_or_to_eol(int n, const char *source)
{
    size_t len = 0;
    ucs4_t _;

    for (uint8_t * it = (uint8_t *)source; it; it = (uint8_t *)u8_next(&_, it)) {
        if (len == n) {
            return it;
        }
        len++;
    }
    return NULL;
}

int break_lines(const int width)
{
    int j = 0;

    for (int i = 0; i < original_lines_total; i++) {
        char *start = original_lines[i];
        // printf("start [%d] \r\n", i);

        int bytes_length = strlen(start);

        if (bytes_length == 0) {
            broken_lines[j][0] = 0;
            // printf("broken_lines[%d] = '%s'\r\n", j, broken_lines[j]);
            j++;
        }

        char *last = &(start[bytes_length-1]);
        // printf("start [%p]  last [%p]\r\n", start, last);

        while (start < last) {
            // printf("j = %d \r\n", j);

            char *finish =
                (char *) skip_n_unicode_chars_or_to_eol(width, start);

            if (finish == NULL) {
                finish = last + 1;
            }        

            strncpy(broken_lines[j], start, finish - start );
            // printf("broken_lines[%d] = '%s'\r\n", j, broken_lines[j]);

            j++;

            if (finish == NULL) {
                start = (char *) skip_n_unicode_chars_or_to_eol(1, finish);
            } else {
                start = finish;            
            }
        }
    }
    return j;
}

void fit_in_available_screen()
{
    struct winsize winsz = get_winsize();
    // printf("running in window size: %d rows / %d columns\n",
    //         winsz.ws_row, winsz.ws_col);

    int broken_lines_total = break_lines(winsz.ws_col);

    pad = newpad(broken_lines_total, winsz.ws_col);

    print_previous_lines(broken_lines_total);
}

static void init_colors()
{
    start_color();
    init_pair(GREY_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(ERROR_PAIR, COLOR_RED + 8, COLOR_YELLOW + 8);
    init_pair(GOOD_PAIR, COLOR_GREEN + 8, COLOR_BLACK);
    clear();
}

static void load_file()
{
    FILE *fp;
    fp = stdin;

    if (fp == NULL) {
        perror("Failed: ");
        exit(1);
    }

    char buffer[MAX_LEN];

    int i = 0;
    while (fgets(buffer, MAX_LEN - 1, fp)) {

        // Remove trailing newline
        buffer[strcspn(buffer, "\n")] = 0;

        strcpy(original_lines[i], buffer);
        i++;
        if (i > 32767) {
            perror("Failed file too long");
            exit(1);
        }
        // if (strlen(buffer) > longest_line) {
        //     longest_line = strlen(buffer);
        // }
    }
    fclose(fp);

    freopen("/dev/tty", "rw", stdin);
   
    original_lines_total = i;
}


// void setup_signals()                                            
// {       
//   struct sigaction new_sig_action;                                            
//   new_sig_action.sa_handler = sig_handler;                
//   sigemptyset (&new_sig_action.sa_mask);                                      
//   new_sig_action.sa_flags = 0;                                                
//   sigaction (SIGWINCH, NULL, &old_sig_action_);                               
//   if (old_sig_action_.sa_handler != SIG_IGN)                                  
//   {                                                                           
//     sigaction (SIGWINCH, &new_sig_action, NULL);                              
//   }                                                                           
// }

int main(void)
{
    logger = fopen("retype.log", "a+");

    signal(SIGWINCH, sig_handler);

    // Locale has to be set before the call to iniscr()
    setlocale(LC_ALL, "");
    initscr();

    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    init_colors();

    load_file();

    // print_grey(1, 3, "original_lines_total");
    refresh();

    fit_in_available_screen();
    overtype_last_line();



    getch();
    endwin();
    exit(0);
}
