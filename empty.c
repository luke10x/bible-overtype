#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <string.h>

#include "./src/menu.h"
#include "./src/status.h"
#include "./src/common.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define NUMBER_OF_BOOKS       66

bookinfo_t all_books[NUMBER_OF_BOOKS] = {
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

unsigned short old_selected_index;

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
}

static void init_screen()
{
#ifdef XCURSES
    Xinitscr(argc, argv);
#else
    initscr();
#endif
    if (has_colors()) {
        init_colors();
    }
    clear();
    nl();
    noecho();
    curs_set(0);
    timeout(0);
    keypad(stdscr, TRUE);
}

struct winsize winsz;
int resized = 0;

status_t *statusbar;
menu_t *book_menu;

void get_winsize()
{
#ifdef EMSCRIPTEN
    winsz.ws_col = 80;
    winsz.ws_row = 24;
#else
    struct winsize new_winsz;
    ioctl(0, TIOCGWINSZ, &new_winsz);

    if (new_winsz.ws_col != winsz.ws_col || new_winsz.ws_row != winsz.ws_row) {
        winsz = new_winsz;
        resized = 1;
    }
#endif
}

static void loop_to_select_book()
{
    char ch = getch();
    if (ch == -1 && !resized)
        return;
    if (ch == 255 && !resized)
        return;                 // For my Little Endian machine mainly
    if (ch == 27) {
        curs_set(1);
        endwin();
#ifdef EMSCRIPTEN
        emscripten_cancel_main_loop();
#endif
        return;
    }

    get_winsize();

    old_selected_index = menu_get_selected_index(book_menu);
    int old_delta = menu_get_delta(book_menu);

    menu_handle_key(book_menu, ch);
    if (status_handle_key(statusbar, ch, book_menu)) {
        resized = 1;
    }

    char *search_term = status_get_search_term(statusbar);

    menu_filter(book_menu, search_term);
    menu_recalculate_dims(book_menu, winsz);

    if (resized || (old_delta != menu_get_delta(book_menu))) {
        resized = 0;
        clear();

        menu_render(book_menu, winsz);
    } else {
        menu_fast_render(book_menu, old_selected_index, winsz);
    }
    status_render(statusbar, winsz);
}

int main(int argc, char *argv[])
{
    init_screen();

    statusbar = status_create();
    char msg[20] = "Enter book:";
    status_set_msg(statusbar, (char *) &msg);
    book_menu = menu_create((mitem_t *) & all_books, NUMBER_OF_BOOKS,
                            sizeof(bookinfo_t), 20);
    get_winsize();
    menu_recalculate_dims(book_menu, winsz);
    resized = 1;
#ifdef EMSCRIPTEN
    emscripten_set_main_loop(loop_to_select_book, 1000 / 50, FALSE);
#else
    for (;; loop_to_select_book())
        napms(50);
#endif
    return 9;
}
