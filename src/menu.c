#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <string.h>

#include "colors.h"

typedef struct menu_t menu_t;

struct bookinfo_t {
    unsigned short int id;
    unsigned short int chapters;
    char *title;
};
typedef struct bookinfo_t bookinfo_t;
typedef union mitem_t {
    bookinfo_t bookinfo;
    unsigned int chapter;
} mitem_t;

// TODO move them to utils
char *strlwr(char *input)
{
    char *str = strdup(input);
    char *p = (char *) str;

    while (*p) {
        *p = tolower((char) *p);
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

struct menu_t {
    const mitem_t *all_items;   // This is the source of the data
    mitem_t *filtered_items;
    size_t item_size;
    unsigned short all_items_count; // How many unfiltered items there are
    unsigned short filtered_items_count;    // How many left after filtering
    char *search_term;
    unsigned short item_format_len; // Item label (number of multibyte chars + '\0')
    unsigned short selected_index;  // Currently selected item
    unsigned short is_done;

    // dims
    unsigned short visible_columns; // how many columns of items the window can fit
    unsigned short height;      // in how many rows the items will be arranged
    unsigned short delta;       // how many columns (or rows) skipped before starting to show
    unsigned short hpadding;    // space left free at the sides of the window
    unsigned short vpadding;    // space left free at the bottom and the top of the window

    // Should be extracted to canvas
    WINDOW *pad;
};

void menu_filter(menu_t * self, char *search)
{
    self->search_term = search;
    mitem_t *selected_ptr = &self->filtered_items[self->selected_index];
    int selected_is_in_new_set = 0;
    self->filtered_items_count = 0; // Will be increased during this function
    char *item_label = malloc(self->item_format_len * sizeof(char));
    for (int i = 0; i < self->all_items_count; i++) {
        // printf("f i = %d\r\n", i);

        char chapter_label[20];

        // TODO here it is now assuming is a book item,
        // but it also can be chapter item
        if (self->item_size == sizeof(int)) {
            const int chapter = self->all_items[i].chapter;
            sprintf((char *) &chapter_label, "%3d", chapter);
        } else {
            const bookinfo_t book = self->all_items[i].bookinfo;
            sprintf(item_label, "%2d. %-15s", book.id, book.title);
        }
        // TODO separate string utils module
        char *found = strstr(strlwr(item_label),
                             strlwr(search));

        if (found != NULL) {
            self->filtered_items[self->filtered_items_count] =
                self->all_items[i];
            if (selected_ptr ==
                &self->filtered_items[self->filtered_items_count]) {
                self->selected_index = self->filtered_items_count;
                selected_is_in_new_set = 1;
            }
            self->filtered_items_count++;
        }
    }
    if (!selected_is_in_new_set && self->filtered_items_count > 0) {
        self->selected_index = 0;
    }
}

// Should be exposed through .h?
unsigned short _get_pad_width(menu_t * self)
{
    int columns = (int) (self->filtered_items_count / self->height) + 1;
    return columns * self->item_format_len;
}


// TODO pass winsz or better canvas as a param
void menu_recalculate_dims(menu_t * self, struct winsize winsz)
{
    self->visible_columns = ((int) (winsz.ws_col / self->item_format_len) - 1);
    if ((winsz.ws_col % self->item_format_len) > 0) {
        self->visible_columns++;
    }
    self->height = (int) (self->filtered_items_count / self->visible_columns);

    if ((self->filtered_items_count % self->visible_columns) > 0) {
        self->height++;
    }
    if (self->height > winsz.ws_row - 1) {
        self->height = winsz.ws_row - 1;
    }

    while (((int) (self->selected_index / self->height)) >=
           ((int) (self->delta + self->visible_columns))) {
        self->delta++;
    }
    while (((int) (self->selected_index / self->height)) < self->delta) {
        self->delta--;
    }

    int visible_width = self->visible_columns * self->item_format_len;
    if (self->filtered_items_count < self->visible_columns) {
        visible_width = self->filtered_items_count * self->item_format_len;
    }

    self->hpadding = 0;
    self->vpadding = 0;
    if (winsz.ws_col > visible_width) {
        self->hpadding = (int) ((winsz.ws_col - visible_width) / 2);

        if ((winsz.ws_col - _get_pad_width(self)) % 2) {
            self->hpadding++;
        }
    }

    self->vpadding = (int) ((winsz.ws_row - self->height) / 2);
}

void menu_recalculate_dims_vert(menu_t * self, struct winsize winsz)
{
    // differ
    self->visible_columns = winsz.ws_row - 1;
    self->height = (int) (self->filtered_items_count * (self->item_format_len + 2)/ (self->visible_columns));

    // if ((self->filtered_items_count % self->visible_columns) > 0) {
    //     self->height++;
    // }
    if (self->height > (int)( winsz.ws_col / self->item_format_len) - 1) {
        self->height = (int)( winsz.ws_col / self->item_format_len) - 1;
    }
    // /differ

    while (((int) (self->selected_index / self->height)) >=
           ((int) (self->delta + self->visible_columns))) {
        self->delta++;
    }
    while (((int) (self->selected_index / self->height)) < self->delta) {
        self->delta--;
    }

    int visible_width =  (int)(self->filtered_items_count / self->height); 
    // if (self->filtered_items_count < self->visible_columns) {
    //     visible_width = self->filtered_items_count * self->item_format_len;
    // }

    self->hpadding = 0;
    self->vpadding = 0;
    // endwin();
    // printf("vw = %d", visible_width);
    // exit(1);
    if (winsz.ws_row > visible_width) {
        self->vpadding = (int) ((winsz.ws_row - visible_width) / 2);

    //     if ((winsz.ws_row - _get_pad_width(self)) % 2) {
    //         self->vpadding++;
    //     }
    }

    self->hpadding = (int) ((winsz.ws_col - self->height * self->item_format_len) / 2);
}


void write_here(menu_t * self, const int row, const int col, int color_pair,
                char *str, struct winsize winsz)
{
    wmove(self->pad, row, col);
    wattron(self->pad, COLOR_PAIR(color_pair));
    waddstr(self->pad, str);
    wattroff(self->pad, COLOR_PAIR(color_pair));

    refresh();

    wrefresh(self->pad);
    prefresh(self->pad, 0, 0, self->vpadding, self->hpadding,
             winsz.ws_row - 1 - self->vpadding, winsz.ws_col - self->hpadding);
}

// Perhaps it is already outside of this class business to draw it
void _draw_one_book(menu_t * self, int y, int x, mitem_t item, int key,
                    struct winsize winsz)
{
    /* FIXME depends where this code will ter be moved,
     * it may have access to the global constant
     */
    int fmtlen = self->item_format_len;

    char s[fmtlen];
    if (self->item_size == sizeof(int)) {
        sprintf(s, "%d", item.chapter);
    } else {
        bookinfo_t book = item.bookinfo;
        sprintf(s, "%2d. %-15s", book.id, book.title);

    }

    int color_pair = PAIR_BOOK;
    if (key == self->selected_index) {
        color_pair = PAIR_BOOK_SELECTED;
    }

    write_here(self, y, x * fmtlen, color_pair, s, winsz);


    // Uncomment this later TODO (need to think how to pass along the search term)
    int pos = strpos(strlwr(s), strlwr(self->search_term));

    char *highlighted = malloc(strlen(self->search_term) + 1);
    highlighted[strlen(self->search_term)] = 0;
    memcpy(highlighted, s + pos, strlen(self->search_term));

    int color_pair_search = PAIR_SEARCH_HIGHLIGHT;
    if (key == self->selected_index) {
        color_pair_search = PAIR_SEARCH_SELECTED;
    }

    write_here(self, y, x * fmtlen + pos, color_pair_search,
               highlighted, winsz);

}

void menu_render(menu_t * self, struct winsize winsz)
{
    delwin(self->pad);

    self->pad = newpad(winsz.ws_row - 1, winsz.ws_col);
    if (self->pad == NULL) {
        char *s = malloc(300);
        sprintf(s, "Pad is not set newpad(%d, %d); failed", winsz.ws_row - 1,
                winsz.ws_col);
        perror(s);
        endwin();
        exit(1);
    }

    scrollok(self->pad, 1);
    wclear(self->pad);

    int i = self->delta * self->height;
    for (int x = 0; x < self->visible_columns; x++) {
        for (int y = 0; y < self->height; y++) {
            if (i >= self->filtered_items_count)
                break;

            // Again book specific...
            const mitem_t item = self->filtered_items[i];
            _draw_one_book(self, y, x, item, i, winsz);

            i++;
        }
    }
}

void menu_render_vert(menu_t * self, struct winsize winsz)
{
    delwin(self->pad);

    self->pad = newpad(winsz.ws_row - 1, winsz.ws_col);
    if (self->pad == NULL) {
        char *s = malloc(300);
        sprintf(s, "Pad is not set newpad(%d, %d); failed", winsz.ws_row - 1,
                winsz.ws_col);
        perror(s);
        endwin();
        exit(1);
    }

    scrollok(self->pad, 1);
    wclear(self->pad);

    int i = self->delta * self->height;
        for (int x = 0; x < self->visible_columns; x++) {
    for (int y = 0; y < self->height; y++) {
            if (i >= self->filtered_items_count)
                break;

            // Again book specific...
            const mitem_t item = self->filtered_items[i];
            _draw_one_book(self, x, y, item, i, winsz);

            i++;
        }
    }
}

void menu_fast_render(menu_t * self, int old_selected_index,
                      struct winsize winsz)
{
    int y = self->selected_index % self->height;
    int x = (int) (self->selected_index / self->height) - self->delta;
    _draw_one_book(self, y, x,
                   self->filtered_items[self->selected_index],
                   self->selected_index, winsz);

    y = old_selected_index % self->height;
    x = (int) (old_selected_index / self->height) - self->delta;
    _draw_one_book(self, y, x,
                   self->filtered_items[old_selected_index],
                   old_selected_index, winsz);
}

void menu_fast_render_vert(menu_t * self, int old_selected_index,
                      struct winsize winsz)
{
    int x = self->selected_index % self->height;
    int y = (int) (self->selected_index / self->height) - self->delta;
    _draw_one_book(self, y, x,
                   self->filtered_items[self->selected_index],
                   self->selected_index, winsz);

    x = old_selected_index % self->height;
    y = (int) (old_selected_index / self->height) - self->delta;
    _draw_one_book(self, y, x,
                   self->filtered_items[old_selected_index],
                   old_selected_index, winsz);
}

void menu_handle_key(menu_t * self, char ch)
{
    if (ch == 2) {
        if ((self->selected_index % self->height) < self->height - 1
            && (self->selected_index + 1) < self->filtered_items_count) {
            self->selected_index++;
        }
    } else if (ch == 3) {
        if ((self->selected_index % self->height) > 0) {
            self->selected_index--;
        }
    } else if (ch == 4) {
        if ((self->selected_index - self->height) >= 0) {
            self->selected_index -= self->height;
        }
    } else if (ch == 5) {
        unsigned short columns =
            (int) (self->filtered_items_count / self->height);
        if (columns * self->height < self->filtered_items_count) {
            columns++;
        }
        if (columns * self->height > self->selected_index + self->height) {
            self->selected_index += self->height;
            if (self->selected_index >= self->filtered_items_count) {
                self->selected_index = self->filtered_items_count - 1;
            }
        }
    }
}


void menu_handle_key_vert(menu_t * self, char ch)
{
    // Down
    if (ch == 2) {

        unsigned short columns =
            (int) (self->filtered_items_count / self->height);
        if (columns * self->height < self->filtered_items_count) {
            columns++;
        }
        if (columns * self->height > self->selected_index + self->height) {
            self->selected_index += self->height; // The move
            if (self->selected_index >= self->filtered_items_count) {
                self->selected_index = self->filtered_items_count - 1;
            }
        }       
                // if ((self->selected_index % self->height) < self->height - 1
        //     && (self->selected_index + 1) < self->filtered_items_count) {
        // }        
        
        //     self->selected_index = self->filtered_items_count - 1;

    } else if (ch == 3) { // up
        if ((self->selected_index - self->height) >= 0) {
            self->selected_index -= self->height;
        }        
    } else if (ch == 4) { // left
        if ((self->selected_index % self->height) > 0) {
            self->selected_index--;
        }

    } else if (ch == 5) { // right
        if ((self->selected_index % self->height) < self->height - 1
            && (self->selected_index + 1) < self->filtered_items_count) {
            self->selected_index++;
        }     
                
    }
}

mitem_t *menu_get_selected_item(menu_t * self)
{
    return &self->filtered_items[self->selected_index];
}

unsigned short menu_get_selected_index(menu_t * self)
{
    return self->selected_index;
}

unsigned short menu_get_delta(menu_t * self)
{
    return self->delta;
}

unsigned short menu_get_filtered_item_count(menu_t * self)
{
    return self->filtered_items_count;
}

unsigned short menu_is_done(menu_t * self)
{
    return self->is_done;
}

void menu_finalize(menu_t * self)
{
    self->is_done = 1;
}

menu_t *menu_create(const mitem_t * all_items,
                    unsigned short all_items_count,
                    size_t item_size, unsigned short item_format_len)
{
    menu_t *self = malloc(sizeof(menu_t));
    self->item_size = item_size;
    self->item_format_len = item_format_len;
    self->all_items = all_items;
    self->all_items_count = all_items_count;

    self->filtered_items = malloc(all_items_count * sizeof(mitem_t));
    self->selected_index = 0;
    self->is_done = 0;

    menu_filter(self, "");

    return self;
}
