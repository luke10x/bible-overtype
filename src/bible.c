#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "./menu.h"
#include "./status.h"
#include "./common.h"
#include "./scripture.h"
#include "./charlie.h"
#include "./overtype.h"

#include "./screen.c"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

unsigned short old_selected_index;

struct winsize winsz;
int resized = 0;

status_t *statusbar;
menu_t *book_menu;
menu_t *chapter_menu;
overtype_t *overtype;


int eye = 0;

static void loop_to_do_overtype()
{
    curs_set(1);

    

    char ch = ovt_try_autotext(overtype, getch());
    check_winsize();
    // ovt_recalculate_size(overtype, winsz);




    if (ch == -1 || ch == 255 || ch == 6 || ch == -102) {

        if (resized) {

            // if (eye) {
            //     endwin();
            //     printf("LOOPED aT ch=%d %dx%d\r\n", ch, winsz.ws_row, winsz.ws_col);
            //     exit(3);
            // }
            // eye = 1;

            check_winsize();
            ovt_recalculate_size(overtype, winsz);

            resized = 0;
            clear();
            ovt_render(overtype, winsz);
        }

        return;
    }

    if (ch == 27) {
        curs_set(1);
        endwin();
#ifdef EMSCRIPTEN
        emscripten_cancel_main_loop();
#endif
        exit(0);
    }
    // curs_set(0);

    // check_winsize()

    if (ovt_handle_key(overtype, ch)) {
        // Whenever the new line is addded and we need to refres all screen
        // That's maybe not exactly true ^^
        clear();
        ovt_render(overtype, winsz);
    }
    // curs_set(1);

}


static void loop_to_select_chapter()
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
        exit(0);
    }
    if (ch == 10) {
        menu_finalize(chapter_menu);
#ifdef EMSCRIPTEN
        emscripten_cancel_main_loop();
#endif
        // endwin();               // End of story starts here.

        bookinfo_t book = menu_get_selected_item(book_menu)->bookinfo;
        chapter_t chapter = menu_get_selected_item(chapter_menu)->chapter;

        uint8_t *blob;
        size_t blob_size;

        blob_size = get_chapter_blob(&blob, book.title, chapter);

        printf("Chapter of size %ld selected.\r\n", blob_size);

        // blob_size = 10;
        for (int i = 0; i < blob_size; i++) {
            printf("%c", blob[i]);
        }

        char *utf8_line = "Tėve mūsų, kuris esi danguje!";
        char *ascii_line = fold2ascii(utf8_line);

        printf("🐤\r\nWAS: %s[%lu]\r\nNOW: %s[%lu]\r\n",
               utf8_line, strlen(utf8_line), ascii_line, strlen(ascii_line)
            );

        overtype = ovt_create(blob);    // Now control is passed to the overtype
        ovt_recalculate_size(overtype, winsz);
        resized = 1;
        
        curs_set(1);

#ifdef EMSCRIPTEN
        emscripten_set_main_loop(loop_to_do_overtype, 1000 / 50, FALSE);
#else

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        for (;;) {
            loop_to_do_overtype();
            napms(50);
        }

        fcntl(STDIN_FILENO, F_SETFL, flags);

#endif
        return;
    }

    check_winsize();

    old_selected_index = menu_get_selected_index(chapter_menu);

    menu_handle_key(chapter_menu, ch);
    if (status_handle_key(statusbar, ch, chapter_menu)) {
        resized = 1;
    }

    char *search_term = status_get_search_term(statusbar);

    menu_filter(chapter_menu, search_term);
    menu_recalculate_dims(chapter_menu, winsz);

    if (resized) {
        resized = 0;
        clear();

        menu_render(chapter_menu, winsz);
    } else {
        menu_fast_render(chapter_menu, old_selected_index, winsz);
    }
    status_render(statusbar, winsz);

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
        exit(0);
    }
    if (ch == 10) {
        menu_finalize(book_menu);
#ifdef EMSCRIPTEN
        emscripten_cancel_main_loop();
#endif
        resized = 1;

        bookinfo_t selected_book = menu_get_selected_item(book_menu)->bookinfo;

        char *m = malloc(30);
        sprintf(m, "%s selected, now choose a chapter:", selected_book.title);
        status_set_msg(statusbar, m);
        status_render(statusbar, winsz);

        mitem_t *inflated_chapters = malloc(MAX_CHAPTER * sizeof(mitem_t));

        const chapter_t *all_chapters = get_this_many_chapters(MAX_CHAPTER);
        for (int i = 0; i < selected_book.chapters; i++)
            inflated_chapters[i].chapter = all_chapters[i];

        chapter_menu = menu_create(inflated_chapters, selected_book.chapters,
                                   sizeof(int), CHAPTER_FORMAT_LEN);

#ifdef EMSCRIPTEN
        emscripten_set_main_loop(loop_to_select_chapter, 1000 / 50, FALSE);
#else
        for (;;) {
            loop_to_select_chapter();
            napms(50);
        }
#endif
        return;
    }

    check_winsize();

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

    book_menu = menu_create((mitem_t *) get_all_books(), NUMBER_OF_BOOKS,
                            sizeof(bookinfo_t), BOOK_FORMAT_LEN);
    check_winsize();
    menu_recalculate_dims(book_menu, winsz);
    resized = 1;
#ifdef EMSCRIPTEN
    emscripten_set_main_loop(loop_to_select_book, 1000 / 50, FALSE);
#else
    for (;;) {
        loop_to_select_book();
        napms(50);
    }
#endif

    return 9;
}
