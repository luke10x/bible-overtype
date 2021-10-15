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
#include <math.h>

#include <unitypes.h>
#include <uniconv.h>
#include <unistdio.h>
#include <unistr.h>
#include <uniwidth.h>
#include <utf8proc.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h> 

typedef struct overtype_t overtype_t;

struct overtype_t {

};
///// Unrefactored ///////////////


#define GREY_PAIR    15
#define GOOD_PAIR    16
#define ERROR_PAIR   17

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

#define MAX_LEN 101
#define MAX_LINES 99999

struct xchar {
    uint8_t type;
    wint_t ch;
    char *str;
};

struct charstack {
    struct xchar ch;
    struct charstack *next;
};

FILE *logger;

WINDOW *pad;

// char original_lines[MAX_LINES][MAX_LEN];
char ** original_lines  = NULL;

char broken_lines[MAX_LINES][MAX_LEN];

int original_lines_total;

int broken_lines_total;

struct winsize winsz;

int cursor = 0;                 // Curent character in the file
// 32;
// 200;
int line = 0;                   // Line under cursor

int column = 0;                 // Column under cursor
                                // (can also be called "line cursor")
int offset = 0;                 // How many lines are hidden in the beginning

int margin = 0;                 // how many columns it is pushed off the left
                                // (to center when text has short lines)
int d1 = 0;
static volatile int resized = 1;

struct charstack *undostack = NULL;

int undostack_size = 0;

uint8_t pending_char[4] = { 0, 0, 0, 0 };   // These are variables for getxchar()

int pending_curr = 0;

void print_grey(const int row, const int col, const char *line)
{
    wmove(pad, row, col);
    wattron(pad, COLOR_PAIR(GREY_PAIR));
    waddstr(pad, line);
    wattroff(pad, COLOR_PAIR(GREY_PAIR));
}

void print_good(const int row, const int col, const char *line)
{
    wmove(pad, row, col);
    wattron(pad, COLOR_PAIR(GOOD_PAIR));
    waddstr(pad, line);
    wattroff(pad, COLOR_PAIR(GOOD_PAIR));
}

void print_bad(const int row, const int col, const char *line)
{
    wmove(pad, row, col);
    wattron(pad, COLOR_PAIR(ERROR_PAIR));
    waddstr(pad, line);
    wattroff(pad, COLOR_PAIR(ERROR_PAIR));
}

static void sig_handler(int sig)
{
    signal(SIGWINCH, SIG_IGN);
    if (SIGWINCH == sig) {
        // resized = 1;
        // char ch = getch();
        // printf("tesize char %d \r\n", ch);
        // endwin;
        // exit(1);
    }
    signal(SIGWINCH, sig_handler);
}

char *combine_bytes(int n, uint8_t * bytes)
{
    char *result = malloc(3);

    result[0] = bytes[0];
    result[1] = bytes[1];
    result[2] = 0;

    return result;
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

size_t char_len(const char *input)
{
    size_t len = 0;
    ucs4_t _;

    for (uint8_t * it = (uint8_t *) input; it; it = (uint8_t *) u8_next(&_, it)) {
        len++;
    }
    len--;
    return len;
}

const uint32_t simplify(const char *input, const int index)
{
    size_t len = char_len(input);
    if (index >= len) {
        return 0;
    }

    const uint32_t *mbcs = u32_strconv_from_locale(input);
    const uint32_t unicode = (int) *(&mbcs[index]);
    return unicode;
}

void get_winsize()
{
    struct winsize new_winsz;
    ioctl(0, TIOCGWINSZ, &new_winsz);

    if (new_winsz.ws_col != winsz.ws_col || new_winsz.ws_row != winsz.ws_row) {
        winsz = new_winsz;
        resized = 1;
    }
}

/**
 * Values to test it with: ė
 */
struct xchar getxchar()
{
    uint8_t ch;
    do {

        get_winsize();
        if (resized) {
            resized = 0;



            // d1++;
            // if (d1 > 1) {
            //     printf("signal handled_%d_time: \r\n" , d1); 
            //     // nocbreak();
            //     // ch = getch();
            //     // printf("char handled_%d_time: \r\n" , d1); 

            // }
            return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_RESIZE };
        }

        halfdelay(5);
        ch = getch();

        if (ch == 255) {
            continue;
        }
        if (ch == 154) {
            continue;           // Dismiss resize artifact
        }

        int seventh = (ch >> 7) & 1;
        int sixth = (ch >> 6) & 1;

        if (seventh && sixth) {
            pending_char[0] = ch;
            pending_char[1] = 0;
            pending_char[2] = 0;
            pending_char[3] = 0;
            pending_curr = 1;

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
                    getch();    // FIXME more reliable way to clean it perhaps???

                    char *str = combine_bytes(pending_curr, pending_char);
                    ch = normalize(simplify(str, 0));

                    return (struct xchar) {
                        .type = XCH_CHAR,
                        .ch = ch,
                        .str = str
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
            return (struct xchar) {.type = XCH_SPECIAL,.ch = XCH_KEY_BACKSPACE
            };
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

        return (struct xchar) {.type = XCH_CHAR,.ch = ch,.str = str };
    }
    while (ch == ERR || ch == KEY_RESIZE);

    return (struct xchar) {.type = XCH_ALT,.ch = '2',.str = "2" };
}

/**
 * This function is to inject nice logging whenever xgetchar() returns something
 */
struct xchar getxchar_()
{
    struct xchar xch = getxchar();

    if (1)                      // comment this line to take advantage of the logs!
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

int is_same(uint32_t expected, struct xchar pressed)
{
    if (expected == 10 && expected == 0) {
        if (pressed.type == XCH_SPECIAL && pressed.ch == XCH_KEY_NEWLINE) {
            return TRUE;
        }
        return FALSE;
    }
    if (pressed.type == XCH_CHAR) {

        uint8_t n_expected = normalize(expected);
        uint8_t n_pressed = normalize(simplify(pressed.str, 0));

        if (n_expected == n_pressed) {
            return TRUE;
        }
    }

    return FALSE;
}


void recalculate_offset()
{
    if (line - offset >= winsz.ws_row) {
        offset = line - winsz.ws_row + 1;
    } else {
        offset = 0;
    }
}

void soft_refresh()
{
    refresh();
    prefresh(pad, offset, 0, 0, margin, winsz.ws_row - 1, winsz.ws_col - 1);
}

void write_here_str(int color_pair, char *str)
{
    wattron(pad, COLOR_PAIR(color_pair));
    waddstr(pad, str);
    wattroff(pad, COLOR_PAIR(color_pair));

    soft_refresh();
}

void print_previous_lines(int number_of_lines)
{
    wclear(pad);
    wmove(pad, 0, 0);

    recalculate_offset();
    soft_refresh();

    for (int i = 0; i < line; i++) {

        print_good(i, 0, broken_lines[i]);
        soft_refresh();

    }

    print_good(line, 0, broken_lines[line]);

    wmove(pad, line, column);

    struct charstack *undostack_copy;
    undostack_copy = undostack;
    int mistake_index = -1;
    while (undostack) {
        mistake_index++;
        struct charstack *temp;
        temp = undostack;
        undostack = undostack->next;
        print_bad(line, column + mistake_index, temp->ch.str);
    }
    undostack = undostack_copy;
    wmove(pad, line, column + undostack_size);

    char *input = broken_lines[line];
    char *output = malloc(101);
    size_t len = 0;
    ucs4_t _;

    for (uint8_t * it = (uint8_t *) input; it; it = (uint8_t *) u8_next(&_, it)) {
        len++;
        if (len > column + undostack_size) {
            sprintf(output, "%s", it);
            break;
        }
    }

    print_grey(line, column + undostack_size, output);
    wmove(pad, line, column + undostack_size);

    soft_refresh();
}

int this_is_lnumber_start(const char *line, int typed)
{
    const uint32_t expected_ch = simplify(line, typed);
    if (expected_ch >= 0x0030 && expected_ch <= 0x0039) {
        return true;
    }
    return false;
}

int this_is_subsequent_space(const char *line, int typed)
{
    if (typed == 0)
        return false;
    const uint32_t previous_ch = simplify(line, typed - 1);
    const uint32_t expected_ch = simplify(line, typed);
    if (previous_ch == 0x0020 && expected_ch == 0x0020) {
        return true;
    }
    return false;
}

bool should_autotext(int now_started, const char *line, int typed,
                     struct charstack *undostack)
{
    if (undostack != 0) {
        return false;
    }
    if (now_started) {
        const uint32_t expected_ch = simplify(line, typed);
        if (expected_ch == 0x0020 ||
            expected_ch == 0x003A ||
            (expected_ch >= 0x0030 && expected_ch <= 0x0039)
            ) {
            return true;
        }
    } else {
        if (this_is_lnumber_start(line, typed)) {
            return true;
        }
        if (this_is_subsequent_space(line, typed)) {
            return true;
        }
    }
    return false;
}

uint8_t *skip_n_unicode_chars_or_to_eol(int n, const char *source)
{
    size_t len = 0;
    ucs4_t _;

    for (uint8_t * it = (uint8_t *) source; it;
         it = (uint8_t *) u8_next(&_, it)) {
        if (len == n) {
            return it;
        }
        len++;
    }
    return NULL;
}

int get_padding(int longest_line, int term_cols)
{
    return (term_cols - longest_line) / 2;
}

int break_lines(const int width)
{
    int j = 0;

    int longest_line = 0;
    for (int i = 0; i < original_lines_total; i++) {
        int this_len = char_len(original_lines[i]);
        if (this_len > longest_line) {
            longest_line = this_len;
        }
        char *start = original_lines[i];

        int bytes_length = strlen(start);

        if (bytes_length == 0) {
            broken_lines[j][0] = 0;
            j++;
        }

        char *last = &(start[bytes_length - 1]);

        while (start <= last) {

            char *finish =
                (char *) skip_n_unicode_chars_or_to_eol(width, start);

            if (finish == NULL) {
                finish = last + 1;
            }

            strncpy(broken_lines[j], start, finish - start);
            for (int z = 0; z < MAX_LEN; z++)
                broken_lines[j][z] = 0;
            strncpy(broken_lines[j], start, finish - start);
            broken_lines[j][finish - start] = 0;

            j++;

            if (finish == NULL) {
                start = (char *) skip_n_unicode_chars_or_to_eol(1, finish);
            } else {
                start = finish;
            }
        }
    }

    margin = get_padding(longest_line, width);
    broken_lines_total = j;
    return j;
}

void fit_in_available_screen()
{
    int broken_lines_total = break_lines(winsz.ws_col - 1);

    clear();

    pad = newpad(broken_lines_total, winsz.ws_col - margin);
    recalculate_offset();
    soft_refresh();

    wmove(pad, line, column);

    int chars_in_lines = 0;
    line = -1;
    size_t linesize = 0;
    while (chars_in_lines <= cursor) {
        line++;
        linesize = char_len(broken_lines[line]);    // + 1; // because newlines add at least 1

        chars_in_lines += linesize;
    }
    int chars_in_previous_lines = chars_in_lines - linesize;

    column = cursor - chars_in_previous_lines;
    wmove(pad, line, column);

    recalculate_offset();
    soft_refresh();

    print_previous_lines(line);

    recalculate_offset();
    soft_refresh();
}

void overtype_current_line()
{
    wmove(pad, line, column);

    recalculate_offset();
    soft_refresh();

    bool autotext_started = false;
    do {
        uint32_t expected_ch;
        size_t len;
        char str[MAX_LEN];

        expected_ch = simplify(broken_lines[line], column);
        sprintf(str, "%lc", expected_ch);

        autotext_started =
            should_autotext(autotext_started, broken_lines[line], column,
                            undostack);

        struct xchar xch = autotext_started ? (struct xchar) {
            .type = XCH_CHAR,
            .ch = normalize(expected_ch),
            .str = str
        }
        : getxchar_();

        len = char_len(broken_lines[line]);

        switch (xch.type) {
        case XCH_SPECIAL:
            switch (xch.ch) {
            case XCH_KEY_ESC:
                endwin();

                while (undostack) {

                    // Pop from undo stack
                    struct charstack *temp;
                    temp = undostack;
                    undostack = undostack->next;
                    undostack_size--;

                    printf("Undo: %s ,'%c' %p (%d), len = %zu\r\n",
                           temp->ch.str, temp->ch.ch, temp->ch.str, temp->ch.ch,
                           strlen(temp->ch.str));
                }

                exit(0);
                break;
            case XCH_KEY_RESIZE:
                fit_in_available_screen();

                continue;
            case XCH_KEY_NEWLINE:
                if (column == len && undostack == 0) {

                    line++;
                    column = 0;
                    print_grey(line, column, broken_lines[line]);
                    wmove(pad, line, 0);

                    recalculate_offset();
                    soft_refresh();
                }

                break;
            case XCH_KEY_BACKSPACE:
                if (undostack == NULL) {
                    continue;
                }

                int last_char_pos = column + undostack_size - 1;

                // Pop from undo stack
                // struct charstack *temp;
                // temp = undostack;
                undostack = undostack->next;
                undostack_size--;

                uint32_t correct_ch =
                    simplify(broken_lines[line], last_char_pos);
                if (correct_ch == 0) {
                    correct_ch = 32;
                }
                char str[MAX_LEN] = { 0, 0, 0, 0, 0 };
                sprintf(str, "%lc", correct_ch);

                print_grey(line, last_char_pos, str);

                wmove(pad, line, last_char_pos);

                soft_refresh();
                break;

            default:
                printf("Special: %d \r\n", xch.ch);
            }
            break;

        case XCH_CHAR:
            if (strlen(xch.str) == 0) {
                continue;
            }
            if (is_same(expected_ch, xch) && undostack == NULL) {
                write_here_str(GOOD_PAIR, str);

                cursor++;
                column++;
            } else {
                if (margin + column + undostack_size >= winsz.ws_col - 1)
                    break;

                struct charstack *nptr = malloc(sizeof(struct charstack));
                nptr->ch = xch;
                nptr->next = undostack;
                undostack = nptr;
                undostack_size++;

                write_here_str(ERROR_PAIR, xch.str);
            }

            break;
        }

    } while (line < broken_lines_total);

}

static void _init_colors()
{
    // start_color();
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
    }
    fclose(fp);

    freopen("/dev/tty", "rw", stdin);

    original_lines_total = i;
}

int it_was_called_main_before(void)
{
    FILE *pidfile = fopen("overtype.pid", "wx");
    fprintf(pidfile, "%ld", (long) getpid());
    fclose(pidfile);

    // signal(SIGWINCH, sig_handler);

    // Locale has to be set before the call to iniscr()
    setlocale(LC_ALL, "");

    load_file();

    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    _init_colors();

    time_t start_time;
    time_t end_time;
    start_time = time(NULL);

    overtype_current_line();

    end_time = time(NULL);
    const double diff_t = difftime(end_time, start_time);
    const int all_seconds = floor(diff_t);
    const int minutes = (int) floor(all_seconds / 60);
    const int seconds = all_seconds % 60;

    printf("Time: %d:%02d\r\n", minutes, seconds);
    nocbreak();
    getch();
    endwin();
    printf("Time: %d:%02d\r\n", minutes, seconds);

    exit(0);
}

void _load_blob(uint8_t *blob) {

    // char buffer[MAX_LEN];
    int count = 0;
char *pch;


    // char *data = (char *)blob
    pch = strtok ((char *)blob,"\n");
    while (pch != NULL)
    {
        original_lines = (char*)realloc(original_lines, sizeof(char*)*(count+1));
        original_lines[count] = (char*)malloc(strlen(pch)+1);
        strcpy(original_lines[count], pch);
        count++;
        pch = strtok (NULL, "\n");
    }

    original_lines_total = count;
}
/////////// clean api ///////////////
overtype_t *ovt_create(uint8_t *blob) {
  overtype_t *self = malloc(sizeof(overtype_t));

  _load_blob(blob);
  _init_colors();


  return self;
}

int ovt_handle_key(overtype_t *self, char ch) {

  int should_re_render = 1;
  return should_re_render;
}

void ovt_recalculate_size(overtype_t *self, struct winsize winsz) {
}

void ovt_render(overtype_t *self, struct winsize winsz) {}