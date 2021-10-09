#include <locale.h>
#include <curses.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include<ctype.h>

#define PAIR_STATUS           1
#define PAIR_BOOK             2
#define PAIR_BOOK_SELECTED    3
#define PAIR_BOOK_HIGHLIGHT   4
#define PAIR_BOOK_DISABLED    5
#define PAIR_BOOK_SECTION     6
#define PAIR_SEARCH           7
#define PAIR_SEARCH_HIGHLIGHT 8

#define MAX_BOOK_TITLE_LEN    20
#define NUMBER_OF_BOOKS       66
#define LAST_HEBREW_SCRIPTURE 39
#define BOOK_FORMAT_LEN       20

struct book {
    unsigned short int id;
    char *title;
};

struct winsize winsz;
char ch;
int resized;
WINDOW *pad;
WINDOW *bar;
int selected = 0;
int old_selected = 0;
int padding = 0;
int vpadding = 0;
int height;
char message[100];
int delta = 0;
int pad_width;
int visible_columns;
char search[20];
int search_len = 0;

struct book all_books[NUMBER_OF_BOOKS] = {
    {.id = 1,.title = "Genesis" },
    {.id = 2,.title = "Exodus" },
    {.id = 3,.title = "Leviticus" },
    {.id = 4,.title = "Numbers" },
    {.id = 5,.title = "Deuteronomy" },
    {.id = 6,.title = "Joshua" },
    {.id = 7,.title = "Judges" },
    {.id = 8,.title = "Ruth" },
    {.id = 9,.title = "1 Samuel" },
    {.id = 10,.title = "2 Samuel" },
    {.id = 11,.title = "1 Kings" },
    {.id = 12,.title = "2 Kings" },
    {.id = 13,.title = "1 Chronicles" },
    {.id = 14,.title = "2 Chronicles" },
    {.id = 15,.title = "Ezra" },
    {.id = 16,.title = "Nehemiah" },
    {.id = 17,.title = "Esther" },
    {.id = 18,.title = "Job" },
    {.id = 19,.title = "Psalms" },
    {.id = 20,.title = "Proverbs" },
    {.id = 21,.title = "Ecclesiastes" },
    {.id = 22,.title = "Song of Solomon" },
    {.id = 23,.title = "Isaiah" },
    {.id = 24,.title = "Jeremiah" },
    {.id = 25,.title = "Lamentations" },
    {.id = 26,.title = "Ezekial" },
    {.id = 27,.title = "Daniel" },
    {.id = 28,.title = "Hosea" },
    {.id = 29,.title = "Joel" },
    {.id = 30,.title = "Amos" },
    {.id = 31,.title = "Obadiah" },
    {.id = 32,.title = "Jonah" },
    {.id = 33,.title = "Micah" },
    {.id = 34,.title = "Nahum" },
    {.id = 35,.title = "Habakkuk" },
    {.id = 36,.title = "Zephaniah" },
    {.id = 37,.title = "Haggai" },
    {.id = 38,.title = "Zechariah" },
    {.id = 39,.title = "Malachi" },
    {.id = 40,.title = "Matthew" },
    {.id = 41,.title = "Mark" },
    {.id = 42,.title = "Luke" },
    {.id = 43,.title = "John" },
    {.id = 44,.title = "Acts of the Apostles" },
    {.id = 45,.title = "Romans" },
    {.id = 46,.title = "1 Corinthians" },
    {.id = 47,.title = "2 Corinthians" },
    {.id = 48,.title = "Galatians" },
    {.id = 49,.title = "Ephesians" },
    {.id = 50,.title = "Philippians" },
    {.id = 51,.title = "Colossians" },
    {.id = 52,.title = "1 Thessalonians" },
    {.id = 53,.title = "2 Thessalonians" },
    {.id = 54,.title = "1 Timothy" },
    {.id = 55,.title = "2 Timothy" },
    {.id = 56,.title = "Titus" },
    {.id = 57,.title = "Philemon" },
    {.id = 58,.title = "Hebrews" },
    {.id = 59,.title = "James" },
    {.id = 60,.title = "1 Peter" },
    {.id = 61,.title = "2 Peter" },
    {.id = 62,.title = "1 John" },
    {.id = 63,.title = "2 John" },
    {.id = 64,.title = "3 John" },
    {.id = 65,.title = "Jude" },
    {.id = 66,.title = "Revelation" }
};

struct book books[NUMBER_OF_BOOKS];
int books_len = 0;

char *strlwr(char *input)
{
    char *str = strdup(input);
    unsigned char *p = (unsigned char *) str;

    while (*p) {
        *p = tolower((unsigned char) *p);
        p++;
    }

    return str;
}

int strpos(char *haystack, char *needle)
{
    char *p = strstr(haystack, needle);
    if (p)
        return p - haystack;
    return -1;
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

void write_here(const int row, const int col, int color_pair, char *str)
{
    wmove(pad, row, col);
    wattron(pad, COLOR_PAIR(color_pair));
    waddstr(pad, str);
    wattroff(pad, COLOR_PAIR(color_pair));

    refresh();

    wrefresh(pad);
    prefresh(pad, 0, 0, vpadding, padding,
             winsz.ws_row - 1 - vpadding, winsz.ws_col - padding);
}

void displayStatusLine()
{
    bar = newpad(2, winsz.ws_col);

    if (search_len == 0) {
        wmove(bar, 0, 0);
        wattron(bar, COLOR_PAIR(PAIR_STATUS));
        waddstr(bar, "Select a book using cursor keys or search by name.");
        wattroff(bar, COLOR_PAIR(PAIR_STATUS));
        curs_set(0);
        goto refresh;
    }
    curs_set(1);
    char *str = "Filtered by: ";

    char tpl[10];
    sprintf((char *) &tpl, "%%-%ds", winsz.ws_col - 0);

    char *bg = malloc(winsz.ws_col + 1);
    sprintf(bg, tpl, str);

    wmove(bar, 0, 0);
    wattron(bar, COLOR_PAIR(PAIR_STATUS));
    waddstr(bar, bg);
    wattroff(bar, COLOR_PAIR(PAIR_STATUS));

    wmove(bar, 0, 13);

    wattron(bar, COLOR_PAIR(PAIR_SEARCH));
    waddstr(bar, search);
    wattroff(bar, COLOR_PAIR(PAIR_SEARCH));

refresh:
    refresh();
    wrefresh(pad);
    prefresh(bar, 0, 0, winsz.ws_row - 1, 0, winsz.ws_row - 1, winsz.ws_col);
}

static void init_colors()
{
    start_color();
    init_pair(PAIR_STATUS, COLOR_WHITE, 0);
    init_pair(PAIR_BOOK, COLOR_GREEN + 8, COLOR_BLACK);
    init_pair(PAIR_BOOK_SELECTED, COLOR_BLACK, COLOR_GREEN);
    init_pair(PAIR_BOOK_HIGHLIGHT, COLOR_GREEN + 8, COLOR_BLACK);
    init_pair(PAIR_BOOK_DISABLED, COLOR_GREEN + 8, COLOR_BLACK);
    init_pair(PAIR_BOOK_SECTION, COLOR_WHITE + 8, 0);
    init_pair(PAIR_SEARCH, COLOR_GREEN + 8, COLOR_BLACK);
    init_pair(PAIR_SEARCH_HIGHLIGHT, COLOR_GREEN + 8, COLOR_GREEN);

    clear();
}

void draw_one_book(int y, int x, struct book book, int key)
{
    char s[BOOK_FORMAT_LEN];


    if (book.id == 0) {
        sprintf(s, "%-19s", book.title);
        write_here(y, x * BOOK_FORMAT_LEN, PAIR_BOOK_SECTION, s);
        return;
    }
    sprintf(s, "%2d. %-15s", book.id, book.title);

    int color_pair = PAIR_BOOK;
    if (key == selected) {
        color_pair = PAIR_BOOK_SELECTED;
    }

    write_here(y, x * BOOK_FORMAT_LEN, color_pair, s);

    int pos = strpos(strlwr(s), strlwr(search));

    char *highlighted = malloc(strlen(search) + 1);
    highlighted[strlen(search)] = 0;
    memcpy(highlighted, s + pos, strlen(search));

    write_here(y, x * BOOK_FORMAT_LEN + pos, PAIR_SEARCH_HIGHLIGHT, highlighted);
}

void recalculate_height()
{
    visible_columns = ((int) (winsz.ws_col / BOOK_FORMAT_LEN) - 1);
    if ((winsz.ws_col % BOOK_FORMAT_LEN) > 0) {
        visible_columns++;
    }

    height = (int) (books_len / visible_columns);
    if ((books_len % visible_columns) > 0) {
        height++;
    }
    if (height > winsz.ws_row - 1) {
        height = winsz.ws_row - 1;
    }

    while (((int) (selected / height)) >= ((int) (delta + visible_columns))) {
        delta++;
    }
    while (((int) (selected / height)) < delta) {
        delta--;
    }

    int visible_width = visible_columns * BOOK_FORMAT_LEN;
    if (books_len < visible_columns) {
        visible_width = books_len * BOOK_FORMAT_LEN;
    }

    padding = 0;
    vpadding = 0;
    if (winsz.ws_col > visible_width) {
        padding = (int) ((winsz.ws_col - visible_width) / 2);

        if ((winsz.ws_col - pad_width) % 2) {
            padding++;
        }
    }

    vpadding = (int) ((winsz.ws_row - height) / 2);
}

/**
 * In order to reach the the last books with less selector move
 * we try to have as many columns as the sreen allows.
 * the screen will always allow to have many rows,
 * so it is better to keep the height of the columns low
 * by creating more columns.
 * The same way, if a screen is smaller then required
 * to fit all the books, we will ad more columns at the right of the
 * screen
 */
void drawBooks()
{
    int columns = (int) (books_len / height) + 1;

    delwin(pad);
    pad_width = columns * BOOK_FORMAT_LEN + 0;

    pad = newpad(winsz.ws_row - 1, winsz.ws_col);
    scrollok(pad, 1);
    wclear(pad);

    // while ()
    int i = delta * height;
    for (int x = 0; x < visible_columns; x++) {
        for (int y = 0; y < height; y++) {
            if (i >= books_len)
                break;
            draw_one_book(y, x, books[i], i);

            i++;
        }
    }

    ///////////////////////////////
    // char s[100];
    // sprintf((char *) &s, "vc %d ; ;", visible_columns);
    // write_here(4, 2, PAIR_STATUS, s);
    // char s[100];
    // sprintf((char *)&s, "vc = %d; height = %d, columns = %d; wsrow=%d;",
    //         visible_columns, height, columns, winsz.ws_row);
    // write_here(2, 2, PAIR_STATUS, s);
}

void fast_draw_books()
{
    int y = selected % height;
    int x = (int) (selected / height) - delta;
    draw_one_book(y, x, books[selected], selected);

    y = old_selected % height;
    x = (int) (old_selected / height) - delta;
    draw_one_book(y, x, books[old_selected], old_selected);
}

void redraw()
{
    drawBooks();
    displayStatusLine();
}

void fast_redraw()
{
    fast_draw_books();
    displayStatusLine();
}

filter_books()
{
    int selected_id = books[selected].id;
    int selected_is_in_new_set = 0;
    books_len = 0;
    for (int i = 0; i < NUMBER_OF_BOOKS; i++) {
        char book_label[20];

        sprintf((char *) &book_label, "%2d. %-15s", all_books[i].id,
                all_books[i].title);

        char *found = strstr(strlwr((char *) &book_label),
                             strlwr((char *) &search));
        if (found != NULL) {
            books[books_len] = all_books[i];
            if (selected_id == books[books_len].id) {
                selected = books_len;
                selected_is_in_new_set = 1;
            }
            books_len++;
        }
    }
    if (!selected_is_in_new_set && books_len > 0) {
        selected = 0;
    }
    // printf("viso found %d books\r\n", books_len);
}

int main(void)
{
    // Locale has to be set before the call to iniscr()
    setlocale(LC_ALL, "");

    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    init_colors();

    get_winsize();

    filter_books();

    halfdelay(5);

    while ((ch = getch()) != 27) {
        get_winsize();

        if (ch == 154) {
            resized = 1;
        }
        if ((ch == 255 || ch == -1) && !resized) {
            continue;
        }

        old_selected = selected;
        int old_delta = delta;

        if (ch == 2) {
            if ((selected % height) < height - 1 && (selected + 1) < books_len) {
                selected++;
            }
        } else if (ch == 3) {
            if ((selected % height) > 0) {
                selected--;
            }
        } else if (ch == 4) {
            if ((selected - height) >= 0) {
                selected -= height;
            }
        } else if (ch == 5) {
            if ((selected + height) < books_len) {
                selected += height;
            } else if ((books_len - (books_len % height) > selected)) {
                selected = books_len - 1;
            }
        } else if (((ch > '0' && ch < 'z') || ch == ' ' || ch == '.') && (search_len < 20)) {
            search[search_len] = ch;
            search_len++;
            filter_books();
            if (books_len == 0) {
                search_len--;
                search[search_len] = 0;
                filter_books();
            } else {
                resized = 1;
            }
        } else if ((ch == 7 || ch == 8) && (search_len > 0)) {
            search_len--;
            search[search_len] = 0;
            filter_books();
            resized = 1;
        } else {
            // printf("uncecogniced ch %d, searchle = %d\r\n", ch, search_len);
        }

        recalculate_height();

        if (resized || (old_delta != delta)) {
            resized = 0;
            clear();
            redraw();
        } else {
            fast_redraw();
        }
    }

    nocbreak();
    endwin();
    exit(0);
}
