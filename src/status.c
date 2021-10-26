#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <string.h>

#include "./common.h"
#include "./menu.h"

typedef struct status_t status_t;

struct status_t {
    char *msg;
    char *search_term;
    unsigned short search_len;
    struct winsize winsz;
    WINDOW *bar;
};

status_t *status_create()
{
    status_t *self = malloc(sizeof(status_t));

    self->search_term = malloc(30);
    self->search_len = 0;
    return self;
}

void status_set_msg(status_t * self, char *msg)
{
    self->msg = msg;
    self->search_len = 0;
    // self->search_term[0] = 0;
}

void status_render(status_t * self, struct winsize winsz)
{
    delwin(self->bar);
    self->bar = newpad(2, winsz.ws_col);

    if (self->search_len == 0) {
        wmove(self->bar, 0, 0);
        wattron(self->bar, COLOR_PAIR(PAIR_STATUS));
        waddstr(self->bar, self->msg);
        wattroff(self->bar, COLOR_PAIR(PAIR_STATUS));
        curs_set(0);
    } else {

        curs_set(1);
        char *str = "Filtered by: ";

        char tpl[10];
        sprintf((char *) &tpl, "%%-%ds", winsz.ws_col - 0);

        char *bg = malloc(winsz.ws_col + 1);
        sprintf(bg, tpl, str);

        wmove(self->bar, 0, 0);
        wattron(self->bar, COLOR_PAIR(PAIR_STATUS));
        waddstr(self->bar, bg);
        wattroff(self->bar, COLOR_PAIR(PAIR_STATUS));

        wmove(self->bar, 0, 13);

        wattron(self->bar, COLOR_PAIR(PAIR_SEARCH));
        waddstr(self->bar, self->search_term);
        wattroff(self->bar, COLOR_PAIR(PAIR_SEARCH));
    }

    refresh();
    wrefresh(self->bar);
    prefresh(self->bar, 0, 0, winsz.ws_row - 1, 0, winsz.ws_row - 1,
             winsz.ws_col);
}

unsigned int status_handle_key(status_t * self, char ch, menu_t * menu)
{
    if (((ch >= '0' && ch <= 'z') || ch == ' ' || ch == '.')
        && (self->search_len < 20)) {
        if (menu_get_filtered_item_count(menu) == 1)
            return 0;           // There is only one choice

        self->search_term[self->search_len] = ch;
        self->search_len++;
        menu_filter(menu, self->search_term);
        if (menu_get_filtered_item_count(menu) == 0) {  // Searching for this leaves use with no choice
            self->search_len--;
            self->search_term[self->search_len] = 0;
            menu_filter(menu, self->search_term);
        } else {
            return 1;
        }
    } else if ((ch == 7 || ch == 8) && (self->search_len > 0)) {
        self->search_len--;
        self->search_term[self->search_len] = 0;
        menu_filter(menu, self->search_term);

        return 1;
    }
    return 0;
}

char *status_get_search_term(status_t * self)
{
    self->search_term[self->search_len] = 0;
    return self->search_term;
}
