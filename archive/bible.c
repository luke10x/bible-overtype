#include <locale.h>
#include <curses.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include<ctype.h>
#include <unistd.h>

#define PAIR_STATUS           1
#define PAIR_BOOK             2
#define PAIR_BOOK_SELECTED    3
#define PAIR_BOOK_HIGHLIGHT   4
#define PAIR_BOOK_DISABLED    5
#define PAIR_BOOK_SECTION     6
#define PAIR_SEARCH           7
#define PAIR_SEARCH_HIGHLIGHT 8
#define PAIR_SEARCH_SELECTED  9

#define MAX_BOOK_TITLE_LEN    20
#define NUMBER_OF_BOOKS       66
#define LAST_HEBREW_SCRIPTURE 39
#define BOOK_FORMAT_LEN       20
#define MAX_CHAPTER          150
#define CHAPTER_FORMAT_LEN     5
struct book {
    unsigned short int id;
    unsigned short int chapters;
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
    {.id = 1,.title = "Genesis",.chapters = 50 },
    {.id = 2,.title = "Exodus",.chapters = 40 },
    {.id = 3,.title = "Leviticus",.chapters = 27 },
    {.id = 4,.title = "Numbers",.chapters = 36 },
    {.id = 5,.title = "Deuteronomy",.chapters = 34 },
    {.id = 6,.title = "Joshua",.chapters = 24 },
    {.id = 7,.title = "Judges",.chapters = 21 },
    {.id = 8,.title = "Ruth",.chapters = 4 },
    {.id = 9,.title = "1 Samuel",.chapters = 31 },
    {.id = 10,.title = "2 Samuel",.chapters = 24 },
    {.id = 11,.title = "1 Kings",.chapters = 22 },
    {.id = 12,.title = "2 Kings",.chapters = 25 },
    {.id = 13,.title = "1 Chronicles",.chapters = 29 },
    {.id = 14,.title = "2 Chronicles",.chapters = 36 },
    {.id = 15,.title = "Ezra",.chapters = 10 },
    {.id = 16,.title = "Nehemiah",.chapters = 13 },
    {.id = 17,.title = "Esther",.chapters = 10 },
    {.id = 18,.title = "Job",.chapters = 42 },
    {.id = 19,.title = "Psalms",.chapters = 150 },
    {.id = 20,.title = "Proverbs",.chapters = 31 },
    {.id = 21,.title = "Ecclesiastes",.chapters = 12 },
    {.id = 22,.title = "Song of Solomon",.chapters = 8 },
    {.id = 23,.title = "Isaiah",.chapters = 66 },
    {.id = 24,.title = "Jeremiah",.chapters = 52 },
    {.id = 25,.title = "Lamentations",.chapters = 5 },
    {.id = 26,.title = "Ezekial",.chapters = 48 },
    {.id = 27,.title = "Daniel",.chapters = 12 },
    {.id = 28,.title = "Hosea",.chapters = 14 },
    {.id = 29,.title = "Joel",.chapters = 3 },
    {.id = 30,.title = "Amos",.chapters = 9 },
    {.id = 31,.title = "Obadiah",.chapters = 1 },
    {.id = 32,.title = "Jonah",.chapters = 4 },
    {.id = 33,.title = "Micah",.chapters = 7 },
    {.id = 34,.title = "Nahum",.chapters = 3 },
    {.id = 35,.title = "Habakkuk",.chapters = 3 },
    {.id = 36,.title = "Zephaniah",.chapters = 3 },
    {.id = 37,.title = "Haggai",.chapters = 2 },
    {.id = 38,.title = "Zechariah",.chapters = 14 },
    {.id = 39,.title = "Malachi",.chapters = 4 },
    {.id = 40,.title = "Matthew",.chapters = 28 },
    {.id = 41,.title = "Mark",.chapters = 16 },
    {.id = 42,.title = "Luke",.chapters = 24 },
    {.id = 43,.title = "John",.chapters = 21 },
    {.id = 44,.title = "Acts",.chapters = 28 },
    {.id = 45,.title = "Romans",.chapters = 16 },
    {.id = 46,.title = "1 Corinthians",.chapters = 16 },
    {.id = 47,.title = "2 Corinthians",.chapters = 13 },
    {.id = 48,.title = "Galatians",.chapters = 6 },
    {.id = 49,.title = "Ephesians",.chapters = 6 },
    {.id = 50,.title = "Philippians",.chapters = 4 },
    {.id = 51,.title = "Colossians",.chapters = 4 },
    {.id = 52,.title = "1 Thessalonians",.chapters = 5 },
    {.id = 53,.title = "2 Thessalonians",.chapters = 3 },
    {.id = 54,.title = "1 Timothy",.chapters = 6 },
    {.id = 55,.title = "2 Timothy",.chapters = 4 },
    {.id = 56,.title = "Titus",.chapters = 3 },
    {.id = 57,.title = "Philemon",.chapters = 1 },
    {.id = 58,.title = "Hebrews",.chapters = 13 },
    {.id = 59,.title = "James",.chapters = 5 },
    {.id = 60,.title = "1 Peter",.chapters = 5 },
    {.id = 61,.title = "2 Peter",.chapters = 3 },
    {.id = 62,.title = "1 John",.chapters = 5 },
    {.id = 63,.title = "2 John",.chapters = 1 },
    {.id = 64,.title = "3 John",.chapters = 1 },
    {.id = 65,.title = "Jude",.chapters = 1 },
    {.id = 66,.title = "Revelation",.chapters = 22 },
};

struct book books[NUMBER_OF_BOOKS];
int books_len = 0;

int all_chapters[MAX_CHAPTER] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
    71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
    91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
    101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
    111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
    121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
    131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
    141, 142, 143, 144, 145, 146, 147, 148, 149, 150
};

int chapters[MAX_CHAPTER];
int chapters_len = 0;

struct book *selected_book;

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
        if (selected_book == NULL) {
            waddstr(bar, "Select a book using cursor keys or search by name.");
        } else {
            char *s = malloc(200);
            sprintf(s, "%s selected. Now select a chapter.",
                    selected_book->title);
            waddstr(bar, s);
        }
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
    init_pair(PAIR_SEARCH_HIGHLIGHT, COLOR_WHITE + 8, COLOR_BLACK);
    init_pair(PAIR_SEARCH_SELECTED, COLOR_WHITE + 8, COLOR_GREEN);

    clear();
}

void draw_one_chapter(int y, int x, int chapter, int key)
{
    char s[BOOK_FORMAT_LEN];

    sprintf(s, "%3d", chapter);

    int color_pair = PAIR_BOOK;
    if (key == selected) {
        color_pair = PAIR_BOOK_SELECTED;
    }

    write_here(y, x * CHAPTER_FORMAT_LEN, color_pair, s);

    int pos = strpos(strlwr(s), strlwr(search));

    char *highlighted = malloc(strlen(search) + 1);
    highlighted[strlen(search)] = 0;
    memcpy(highlighted, s + pos, strlen(search));

    int color_pair_search = PAIR_SEARCH_HIGHLIGHT;
    if (key == selected) {
        color_pair_search = PAIR_SEARCH_SELECTED;
    }
    write_here(y, x * CHAPTER_FORMAT_LEN + pos, color_pair_search, highlighted);
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

    int color_pair_search = PAIR_SEARCH_HIGHLIGHT;
    if (key == selected) {
        color_pair_search = PAIR_SEARCH_SELECTED;
    }
    write_here(y, x * BOOK_FORMAT_LEN + pos, color_pair_search, highlighted);
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

void filter_books()
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
}


void filter_chapters()
{
    int selected_id = chapters[selected];
    int selected_is_in_new_set = 0;
    chapters_len = 0;
    for (int i = 0; i < selected_book->chapters; i++) {
        char chapter_label[20];

        sprintf((char *) &chapter_label, "%3d", all_chapters[i]);

        char *found = strstr(strlwr((char *) &chapter_label),
                             strlwr((char *) &search));
        if (found != NULL) {
            chapters[chapters_len] = all_chapters[i];
            if (selected_id == chapters[chapters_len]) {
                selected = chapters_len;
                selected_is_in_new_set = 1;
            }
            chapters_len++;
        }
    }
    if (!selected_is_in_new_set && chapters_len > 0) {
        selected = 0;
    }
}

void recalculate_chapters()
{
    visible_columns = ((int) (winsz.ws_col / CHAPTER_FORMAT_LEN));

    height = (int) (chapters_len / visible_columns) + 1;
    if ((chapters_len % visible_columns) == 0) {
        height--;
    }

    int visible_width = visible_columns * CHAPTER_FORMAT_LEN;
    if (chapters_len < visible_columns) {
        visible_width = chapters_len * CHAPTER_FORMAT_LEN;
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



void redraw_chapters()
{
    int columns = 10;

    delwin(pad);
    pad_width = columns * CHAPTER_FORMAT_LEN + 0;

    pad = newpad(winsz.ws_row - 1, winsz.ws_col);
    scrollok(pad, 1);
    wclear(pad);

    int i = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < visible_columns; x++) {
            if (i >= chapters_len)
                break;
            draw_one_chapter(y, x, chapters[i], i);

            i++;
        }
    }

    displayStatusLine();
}

void fast_redraw_chapters()
{
    int x = selected % visible_columns;
    int y = (int) (selected / visible_columns);
    draw_one_chapter(y, x, chapters[selected], selected);

    x = old_selected % visible_columns;
    y = (int) (old_selected / visible_columns);
    draw_one_chapter(y, x, chapters[old_selected], old_selected);

    displayStatusLine();
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

    while ((ch = getch()) != 10) {
        get_winsize();
        if (ch == 27) {
            endwin();
            exit(0);
        }
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
        } else if (((ch > '0' && ch < 'z') || ch == ' ' || ch == '.')
                   && (search_len < 20)) {
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

    selected_book = &books[selected];
    chapters_len = selected_book->chapters;

    while (search_len > 0) {
        search_len--;
        search[search_len] = 0;
    }

    filter_chapters();

    resized = 1;
    while ((ch = getch()) != 10) {
        get_winsize();
        if (ch == 27) {
            endwin();
            exit(0);
        }
        if (ch == 154) {
            resized = 1;
        }
        if ((ch == 255 || ch == -1) && !resized) {
            continue;
        }

        old_selected = selected;
        if (ch == 2) {
            if ((selected + visible_columns) < chapters_len) {
                selected += visible_columns;
            }
        } else if (ch == 3) {
            if ((selected - visible_columns) >= 0) {
                selected -= visible_columns;
            }
        } else if (ch == 4) {

            if ((selected) % visible_columns > 0) {
                selected--;
            }
        } else if (ch == 5) {
            if (((selected + 1) % visible_columns > 0)
                && (selected + 1) < chapters_len) {
                selected++;
            }
        } else if (ch > '0' && ch < '9' && (search_len < 20)) {
            search[search_len] = ch;
            search_len++;
            filter_chapters();
            if (books_len == 0) {
                search_len--;
                search[search_len] = 0;
                filter_chapters();
            } else {
                resized = 1;
            }
        } else if ((ch == 7 || ch == 8) && (search_len > 0)) {
            search_len--;
            search[search_len] = 0;
            filter_chapters();
            resized = 1;
        }

        recalculate_chapters();

        if (resized) {
            resized = 0;
            clear();
            redraw_chapters();
        } else {
            fast_redraw_chapters();
        }
    }

    nocbreak();
    endwin();

    char s[4];
    sprintf((char *) &s, "%d", chapters[selected]);
    char *args[] = { "python3", "printchapter", selected_book->title, s, NULL };
    if (execvp("python3", args) == -1) {
        printf("\nfailed connection\n");
    }

    printf("Selected: %s %s\r\n", selected_book->title, s);

    exit(0);
}
