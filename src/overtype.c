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
#include <time.h>

#include <utf8proc.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "charlie.h"

typedef struct overtype_t overtype_t;

struct overtype_t {
    bool autotext_started;
    time_t start_time;
    time_t end_time;
    char *title;
};
///// Unrefactored ///////////////


#define GREY_PAIR     15
#define GOOD_PAIR     16
#define ERROR_PAIR    13
#define NEW_GREY_PAIR 12

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
    char *str;
    struct charstack *next;
};

FILE *logger;

WINDOW *pad;

// char original_lines[MAX_LINES][MAX_LEN];
char **original_lines = NULL;

// char broken_lines[MAX_LINES][MAX_LEN];
char **broken_lines = NULL;

int original_lines_total;

int broken_lines_total = 999999;

extern struct winsize winsz;

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
    char *ascii = fold2ascii((char *) input);
    return strlen(ascii);
}

const uint32_t copy_mb_char(const char *input, const int index)
{
    size_t len = char_len(input);
    if (index >= len) {
        return 0;
    }

    return (uint32_t) input[index];

    // const uint32_t *mbcs = u32_strconv_from_locale(input);
    // const uint32_t unicode = (int) *(&mbcs[index]);
    // return unicode;
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

    if (undostack) {
        struct charstack *undostack_copy;
        undostack_copy = undostack;
        int mistake_index = -1;
        while (undostack) {
            mistake_index++;
            struct charstack *temp;
            temp = undostack;
            undostack = undostack->next;
            print_bad(line, column + (undostack_size - mistake_index - 1),
                      temp->str);
        }
        undostack = undostack_copy;
    }

    wmove(pad, line, column + undostack_size);

    // TODO figure out what it is for and rewrite without u8_next
    char *input = (char *) broken_lines[line];
    // char *output = malloc(101);
    // size_t len = 0;
    // // ucs4_t _;

    // for (uint8_t * it = (uint8_t *) input; it; it = (uint8_t *) u8_next(&_, it)) {
    //     len++;
    //     if (len > column + undostack_size) {
    //         sprintf(output, "%s", it);
    //         break;
    //     }
    // }

    // printf("seg: '%s'=input, %d!!!\r\n", input, column + undostack_size);


    for (int i = column + undostack_size; i < winsz.ws_col - margin - 1; i++)
        print_grey(line, i, " ");   // \xc2\xa0

    char *output =
        (char *) skip_n_unicode_chars_or_to_eol(column + undostack_size, input);

    print_grey(line, column + undostack_size, output);
    wmove(pad, line, column + undostack_size);


    soft_refresh();
}

int this_is_lnumber_start(const char *line, int typed)
{
    const char expected_ch = copy_mb_char(line, typed);
    if (expected_ch >= '0' && expected_ch <= '9') {
        return true;
    }
    return false;
}

int this_is_subsequent_space(const char *line, int typed)
{
    if (typed == 0)
        return false;
    const uint32_t previous_ch = copy_mb_char(line, typed - 1);
    const uint32_t expected_ch = copy_mb_char(line, typed);
    if (previous_ch == 0x0020 && expected_ch == 0x0020) {
        return true;
    }
    return false;
}

bool should_autotext(int now_started, const char *line, int typed,
                     struct charstack *undostack)
{

    // printf("\r\n\r\n sould aT? \r\n");
    if (undostack != 0) {
        return false;
    }
    if (now_started) {

        const uint32_t expected_ch = copy_mb_char(line, typed);
        // printf("\r\n\r\n sould aT? \r\n");

        if (expected_ch == ' ' ||
            expected_ch == ':' || (expected_ch >= '0' && expected_ch <= '9')
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

int get_padding(int longest_line, int term_cols)
{
    return (term_cols - longest_line) / 2;
}

void break_blob(int screen_width)
{

    int j = 0;

    int longest_line = 0;

    for (int i = 0; i < original_lines_total; i++) {

        int this_len = 0;
        if (original_lines[i] != NULL) {

            // TODO here it should be width and not length escalated!!!!
            // WARNING this is is a quick wo for a segfault
            this_len = strlen(original_lines[i]);

        } else {
            original_lines[i][0] = '0';
        }
        if (this_len > longest_line) {
            longest_line = this_len;
        }

        linebreaker_t *linebreaker =
            lnbr_create(original_lines[i], screen_width);
        while (true) {
            char *piece = lnbr_take_some(linebreaker);
            if (piece == NULL)
                break;
            broken_lines =
                (char **) realloc(broken_lines, sizeof(char *) * (j + 1));
            broken_lines[j] = piece;
            j++;
        }
    }

    margin = get_padding(longest_line, screen_width);
    broken_lines_total = j;
}

void fit_in_available_screen(struct winsize winsz)
{
}

static void _init_colors()
{
    init_pair(GREY_PAIR, COLOR_WHITE, COLOR_BLACK);
#ifdef EMSCRIPTEN
    init_pair(ERROR_PAIR, COLOR_BLACK, COLOR_BLUE);
#else
    init_pair(ERROR_PAIR, COLOR_BLACK, COLOR_RED + 8);
#endif
    init_pair(GOOD_PAIR, COLOR_GREEN + 8, COLOR_BLACK);
    clear();
}

void _load_blob(uint8_t * blob)
{
    int count = 0;
    char *pch;
    char *data;
    char *tofree;

    tofree = data = strdup((const char *) blob);

    while ((pch = strsep(&data, "\n")) != NULL) {
        original_lines =
            (char **) realloc(original_lines, sizeof(char *) * (count + 1));
        original_lines[count] = (char *) malloc(strlen(pch) + 1);
        strcpy(original_lines[count], pch);
        original_lines[count][strlen(pch)] = 0; // to make all lines end properly
        count++;
    }
    free(tofree);

    original_lines_total = count;
}

/////////// clean api ///////////////
// That night, he went home hungry.

int _is_same(char expected, char pressed)
{
    if (pressed == 10 && expected == 0) {
        true;
    }
    // if (pressed != expected) {

    //     endwin(); printf("preseed %c; expected %c (%d);>\r\n", pressed, expected, expected); exit(1);
    // }

    char *str_expected = malloc(2);
    str_expected[0] = (unsigned char) expected;
    str_expected[1] = 0;

    char *str_pressed = malloc(2);
    str_pressed[0] = pressed;
    str_pressed[1] = 0;

    char *ascii_expected = fold2ascii(str_expected);
    // char *ascii_expected = &str_expected;
    char *ascii_pressed = fold2ascii(str_pressed);

    // if (pressed != expected) {

    //     endwin(); printf("preseed %c; expected %c (%d); pr %s; ex %s;>\r\n", pressed, expected, expected, ascii_pressed, ascii_expected); exit(1);
    // }

    int result = ascii_expected[0] == ascii_pressed[0];
    if (!result) {
    }
    // printf("\r\n     [%c %d == %c %d] \r\n", expected, expected, pressed,
    // pressed);
    return result;
}


overtype_t *ovt_create(uint8_t * blob, char *title)
{
    overtype_t *self = malloc(sizeof(overtype_t));

    _load_blob(blob);
    _init_colors();

    undostack_size = 0;
    undostack = NULL;

    self->autotext_started = 0;

    self->start_time = time(NULL);
    self->end_time = 0;
    self->title = title;

    return self;
}

char ovt_try_autotext(overtype_t * self, char ch)
{
    char expected_ch = broken_lines[line][column];

    self->autotext_started =
        should_autotext(self->autotext_started, broken_lines[line], column,
                        undostack);

    if (self->autotext_started) {
        return expected_ch;
    }

    return ch;
}

int ovt_handle_key(overtype_t * self, char ch)
{
// endwin();
    char *exstr = get_utf_char_at_from_string(column, broken_lines[line]);
    char expected_ch = fold2ascii(exstr)[0];
        const size_t expected_size_in_bytes = strlen(exstr);


    const size_t size_in_bytes = strlen(exstr);
    // printf("sizein bytes = %d \r\n", size_in_bytes);

            //  printf("exstr  '%s sx(%d)' [%u, %d, %d, %d] ;; expected_ch = '%c' (%d) >\r\n", exstr, expected_size_in_bytes, 
            //  (uint8_t) exstr[0], (uint8_t)exstr[1], (uint8_t)exstr[2], (uint8_t)exstr[3], expected_ch, expected_ch); // exit(1);


    // if (pressed != expected) {

    //     endwin(); printf("preseed %c; expected %c (%d);>\r\n", pressed, expected, expected); exit(1);
    // }

    size_t len = char_len(broken_lines[line]);

    if (ch == 10) {
        if (column == len && undostack == 0) {
            wclrtoeol(pad);
            recalculate_offset();
            soft_refresh();

            line++;
            if (line < broken_lines_total) {
                column = 0;
                for (int i = 0; i < winsz.ws_col - margin - 1; i++)
                    print_grey(line, i, " ");   // \xc2\xa0
                print_grey(line, column, broken_lines[line]);
                wmove(pad, line, 0);

                recalculate_offset();
                soft_refresh();
            }
        }
    } else if ((ch == 8 || ch == 7)) {
        if (undostack) {

            int last_char_pos = column + undostack_size - 1;

            // Pop from undo stack
            // struct charstack *temp;
            // temp = undostack;
            undostack = undostack->next;
            undostack_size--;

            // char *exstr = get_utf_char_at_from_string(column, broken_lines[line]);
            // uint32_t correct_ch = broken_lines[line][last_char_pos];
            

            // if (correct_ch == 0) {
            //     correct_ch = 32;
            // }
            // char str[MAX_LEN] = { 0, 0, 0, 0, 0 };
            // sprintf(str, "%c", correct_ch);

            // char str = get_utf_char_at_from_string(column, broken_lines[line]); // 

            // wdelch(pad);
            // wdelch(pad);
            wmove(pad, line, last_char_pos);
            wclrtoeol(pad);

            // This soft-refresh is crucial for WASM,
            // so that the cursor is actually moved before writing
            soft_refresh();

            for (int i = column + undostack_size;
                 i < winsz.ws_col - margin - 1; i++)
                print_grey(line, i, " ");   // \xc2\xa0 

            for (int i = column + undostack_size;
                 i < char_len(broken_lines[line]); i++) {
                
                
                // uint32_t new_ch = broken_lines[line][i];
                // char str[MAX_LEN] = { 0, 0, 0, 0, 0 };
                // sprintf(str, "%c", new_ch);

                char *str = get_utf_char_at_from_string(i, broken_lines[line]);

                print_grey(line, i, str);
            }
            wmove(pad, line, last_char_pos);

            soft_refresh();
        }

    } else {
        char *ch_as_str = malloc(2);
        ch_as_str[0] = ch;
        ch_as_str[1] = 0;

        // char *expected_ch_as_str = malloc(2);
        // expected_ch_as_str[0] = expected_ch;
        // expected_ch_as_str[1] = 0;

        if (_is_same(expected_ch, ch) && undostack == NULL) {
            write_here_str(GOOD_PAIR, exstr);
            cursor ++; //= expected_size_in_bytes;
            column ++; //= expected_size_in_bytes;
            // cursor += expected_size_in_bytes;
            // column ++; // expected_size_in_bytes;
        } else {

            if (margin + column + undostack_size < winsz.ws_col - 1) {


                struct charstack *nptr = malloc(sizeof(struct charstack));

                char *bad_str = malloc(5);
                sprintf(bad_str, "%c", ch);

                nptr->str = bad_str;
                nptr->next = undostack;
                undostack = nptr;
                undostack_size++;

                write_here_str(ERROR_PAIR, ch_as_str);
            }
        }
    }

    int should_re_render = 0;
    return should_re_render;
}

void ovt_recalculate_size(overtype_t * self, struct winsize winsz)
{
    break_blob(winsz.ws_col - 1);


    wmove(pad, line, column);

    int chars_in_lines = 0;
    line = -1;
    size_t linesize = 0;
    while (chars_in_lines <= cursor) {
        line++;
        linesize = char_len(broken_lines[line]);

        chars_in_lines += linesize;
    }
    int chars_in_previous_lines = chars_in_lines - linesize;

    column = cursor - chars_in_previous_lines;
}

void ovt_render(overtype_t * self, struct winsize winsz)
{
    pad = newpad(broken_lines_total+5, winsz.ws_col - margin);

    wmove(pad, line, column);
    recalculate_offset();
    soft_refresh();

    print_previous_lines(line);

    recalculate_offset();
    soft_refresh();
}

int ovt_is_done(overtype_t * self, char *print_this_if_done)
{
    if (line < broken_lines_total) return 0;

    if (self->end_time == 0) {
        self->end_time = time(NULL);

        const double diff_t = difftime(self->end_time, self->start_time);
        const int all_seconds = (int)(diff_t);
        const int minutes = (int)(all_seconds / 60);
        const int seconds = all_seconds % 60;
        
        print_grey(line, 0, "--");

        line++;
        recalculate_offset();
        soft_refresh();

        char *r = malloc(180);
        sprintf(r, "%s completed (%d:%02d).", self->title, minutes, seconds);
        print_grey(line, 0, r);

        line++;
        recalculate_offset();
        soft_refresh();

        if (print_this_if_done != NULL) {
            print_grey(line, 0, "Log activity on:");

            line++;
            recalculate_offset();
            soft_refresh();

            sprintf(
                r,
                "https://bible-track.herokuapp.com/go?c=%s&t=%d+%d",
                print_this_if_done,
                self->start_time,
                all_seconds
            );
            print_grey(line, 0, r);

            line++;
            recalculate_offset();
            soft_refresh();
        }
    }
    return 1;
}
