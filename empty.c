/**
 * emcc -I../emcurses -o rain.html rain.c --pre-js ../emcurses/emscripten/termlib.js ../emcurses/emscripten/libpdcurses.so 
 */
#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <sys/ioctl.h>

#ifdef EMSCRIPTEN
    #include <emscripten.h>
#endif

#define PAIR_STATUS           1
#define PAIR_BOOK             2
#define PAIR_BOOK_SELECTED    3
#define PAIR_BOOK_HIGHLIGHT   4
#define PAIR_BOOK_DISABLED    5
#define PAIR_BOOK_SECTION     6
#define PAIR_SEARCH           7
#define PAIR_SEARCH_HIGHLIGHT 8
#define PAIR_SEARCH_SELECTED  9

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

WINDOW *pad;


struct winsize winsz;
int resized = 0;
int padding = 0;
int vpadding = 0;


void get_winsize()
{
    struct winsize new_winsz;
    ioctl(0, TIOCGWINSZ, &new_winsz);

    if (new_winsz.ws_col != winsz.ws_col || new_winsz.ws_row != winsz.ws_row) {
        winsz = new_winsz;
        resized = 1;
    }
}

static void one_iter()
{
    attron(PAIR_BOOK_SELECTED);
    // addstr("storas");
    attroff(PAIR_BOOK_SELECTED);

    int z = rand() % 3;
    chtype color = COLOR_PAIR(z);
    attrset(color);

    char ch = getch();
    if (ch == -1) return;
    if (ch == 27)
    {
            curs_set(1);
            endwin();
#ifdef EMSCRIPTEN
            emscripten_cancel_main_loop();
#endif
            return;
    }
    
    if (ch > '0' && ch < 'z') {
     mvaddch(2,3, ch);
  }

        char s[20];
        sprintf((char *)&s, "charX: %d", ch);
        addstr(s);
  
}



// void write_here(const int row, const int col, int color_pair, char *str)
// {
//     wmove(pad, row, col);
//     wattron(pad, COLOR_PAIR(color_pair));
//     waddstr(pad, str);
//     wattroff(pad, COLOR_PAIR(color_pair));

//     refresh();

//     wrefresh(pad);
//     prefresh(pad, 0, 0, vpadding, padding,
//              winsz.ws_row - 1 - vpadding, winsz.ws_col - padding);
// }

int main(int argc, char *argv[])
{


    // Locale has to be set before the call to iniscr()
    // setlocale(LC_ALL, "");


#ifdef XCURSES
    Xinitscr(argc, argv);
#else
    initscr();
#endif

    if (has_colors())
    {
        int bg = COLOR_BLUE;
        init_colors();

#if defined(NCURSES_VERSION) || (defined(PDC_BUILD) && PDC_BUILD > 3000)
            if (use_default_colors() == OK)
                bg = -1;
#endif

    }
    clear();




    nl();
    noecho();
    curs_set(0);
    timeout(0);
    keypad(stdscr, TRUE);



#ifdef EMSCRIPTEN
    emscripten_set_main_loop(one_iter, 1000/50, FALSE);
#else
    for (;;)
    {
        one_iter();
        napms(50);
    }
#endif

return 9;




}
