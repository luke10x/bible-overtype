extern struct winsize winsz;
extern int resized;

void check_winsize()
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
