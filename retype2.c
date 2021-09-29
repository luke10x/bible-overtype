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

int cursor = 0;
int line = 0; // Line under cursor
int column = 0; // Column under cursor (can also be called "line cursor")
int offset = 0;


struct xchar {
    uint8_t type;
    wint_t ch;
    char * str;
};

static volatile int resized = 1;

struct charstack {
    struct xchar ch;
    struct charstack *next;
};

struct charstack *undostack = NULL;
int undostack_size = 0;

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
    if (SIGWINCH == sig) {
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
            return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_RESIZE };
        }

        halfdelay (1);
        ch = getch();

        if (ch == 255) {
            continue;
        }

        if (ch == 255) ch = getch(); // FIXME where is this coming from? If ever..

        int seventh = (ch >> 7) & 1;
        int sixth = (ch >> 6) & 1;

        if (seventh && sixth) {
            pending_char[0] = ch;
            pending_char[1] = 0;
            pending_char[2] = 0;
            pending_char[3] = 0;
            pending_curr = 1;

            // printf("zeroth byte %d \r\n", ch);
            int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
            do {
                uint8_t ch = getch();

                int seventh = (ch >> 7) & 1;
                int sixth = (ch >> 6) & 1;
                if (seventh && !sixth) {
                    // printf("byte nr %d %d\r\n", pending_curr, ch);

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

            if (ch == 255) {
                fcntl(STDIN_FILENO, F_SETFL, flags);
                return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_ESC };
            }

            fcntl(STDIN_FILENO, F_SETFL, flags);
            return (struct xchar) {.type = XCH_ALT,.ch = ch };
        }

        char *str = malloc(2);
        str[0] = ch;
        str[1] = 0;
        char *result = malloc(3);

        return (struct xchar) {.type = XCH_CHAR,.ch = ch,.str = str};
    }
    while (ch == ERR || ch == KEY_RESIZE);
}

/**
 * This function is to inject nice logging whenever xgetchar() returns something
 */
struct xchar getxchar_()
{
    struct xchar xch = getxchar();

    if (1) // comment this line to take advantage of the logs!
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

size_t char_len(char *input) {
    size_t len = 0;
    ucs4_t _;

    for (uint8_t * it = (uint8_t *)input; it; it = (uint8_t *)u8_next(&_, it)) {
        len++;
    }
    len--;
    return len;
}

const uint32_t simplify(const char *input, const int index)
{
    size_t len = char_len(input);
    if (index >= len) {
        return 0
        ;
    }

    const uint32_t *mbcs = u32_strconv_from_locale(input);
    const uint32_t unicode = (int) *(&mbcs[index]);
    return unicode;
}

int is_same(uint32_t expected, struct xchar pressed) {
    // if (pressed.type == XCH_SPECIAL && pressed.ch == XCH_KEY_NEWLINE) {
        // printf("expexted = %d\r\n", expected);
    // }
    if (expected == 10 && expected == 0) {
        if (pressed.type == XCH_SPECIAL && pressed.ch == XCH_KEY_NEWLINE) {
            return TRUE;
        }
        return FALSE;
    }
    if (pressed.type == XCH_CHAR) {

        uint8_t n_expected = normalize(expected);

        uint8_t n_pressed;
        if (pressed.ch < 255) {
            n_pressed = pressed.ch;
        } else {
            n_pressed = normalize(simplify(pressed.str, 0));
        }

        // printf("eq(%d, %d)\r\n", n_expected, n_pressed);
        if (n_expected == n_pressed) {
            return TRUE;
        }
    }

    return FALSE;
}

void write_here(int color_pair, char * str) {
    wattron(pad, COLOR_PAIR(color_pair));
    waddstr(pad, str);
    wattroff(pad, COLOR_PAIR(color_pair));
    refresh();
    struct winsize w = get_winsize();
    prefresh(pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);
}

void print_previous_lines(int number_of_lines)
{
    for(int i = 0; i < line+1; i++) {
        print_grey(line, column, broken_lines[i]);
    }
    wmove(pad, line, column);
    refresh();
    struct winsize w = get_winsize();
    prefresh(pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);
}

void overtype_current_line()
{
    struct charstack *undostack = NULL;

    wmove(pad, line, column);
    refresh();

    struct winsize w = get_winsize();
    prefresh(pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);

    bool autotext_started = false;
    do {

        uint32_t expected_ch;
        size_t len;
        // printf("EXPECTED IS %d <", expected_ch);
        struct xchar xch = getxchar_();


        len = char_len(broken_lines[line]);

        switch (xch.type) {
        case XCH_CHAR:
    
            expected_ch = simplify(broken_lines[line], column);
            char str[80];
            sprintf(str, "%lc", expected_ch);

            if (is_same(expected_ch, xch) && undostack == NULL) {
                write_here(GOOD_PAIR, str);

                cursor++;
                column++;
            } else {
                const struct xchar good_ch = (column < char_len(broken_lines[line]))
                    ? (struct xchar) { .type = XCH_CHAR, .ch = '$', .str = str }
                    : xch;
                getch();

                struct charstack *nptr = malloc(sizeof(struct charstack));
                nptr->ch = good_ch;
                nptr->next = undostack;
                undostack = nptr;
                undostack_size++;

                write_here(ERROR_PAIR, xch.str);
            }

            if (column == len) {
                break; // this is it done. Add nothing to this line...
            }

            break;
        case XCH_SPECIAL:
            switch (xch.ch) {
            case XCH_KEY_ESC:
                printf("<Escape> \r\n");
                break;
            case XCH_KEY_NEWLINE:
                if (column == len && undostack == 0) {
                    cursor++;
                    line++;
                    column = 0;
                    // now move the visual cursor down, but first scol a bit if necessary
                    
                    // ok move the cursor now
                    // wmove(pad, line - offset, 0);
                    print_grey(line, column, broken_lines[line]);
                    wmove(pad, line, 0);

                    refresh();
                    struct winsize w = get_winsize();


                    if (line - offset >= w.ws_row) {
                        offset++;
                    }
                    prefresh(pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);
                } else if (column < len) {


                    // const struct xchar good_ch = (column < char_len(line))
                    //     ? (struct xchar) { .type = XCH_CHAR, .ch = '$', .str = str }
                    //     : xch;
                    // struct charstack *nptr = malloc(sizeof(struct charstack));
                    // nptr->ch = good_ch;
                    // nptr->next = undostack;
                    // undostack = nptr;
                    // undostack_size++;

                    // write_here(ERROR_PAIR, "¶");
                }
            
                break;
            case XCH_KEY_BACKSPACE:
                // printf("<Backspace> \r\n");
                 if (undostack == NULL) {
                    continue;
                }

                int last_char_pos = column+undostack_size-1;

                // Pop from undo stack
                struct charstack *temp;
                temp = undostack;
                undostack = undostack->next;
                undostack_size--;

                uint32_t correct_ch = simplify(broken_lines[line], last_char_pos);
                if (correct_ch == 0) {
                    correct_ch = 32;
                }
                char str[80] = {0, 0, 0 ,0 ,0};
                sprintf(str, "%lc", correct_ch);

                print_grey(line, last_char_pos, str);
              
                wmove(pad, line, last_char_pos);

                refresh();
                prefresh(pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);          

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
            j++;
        }

        char *last = &(start[bytes_length-1]);

        while (start < last) {

            char *finish =
                (char *) skip_n_unicode_chars_or_to_eol(width, start);

            if (finish == NULL) {
                finish = last + 1;
            }        

            strncpy(broken_lines[j], start, finish - start);
            for (int z = 0; z < MAX_LEN; z++) broken_lines[j][z] = 0;
            strncpy(broken_lines[j], start, finish - start);
            broken_lines[j][finish - start ] = 0;

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
    // refresh();


    // fit_in_available_screen();
    overtype_current_line();

    getch();
    endwin();
    exit(0);
}
