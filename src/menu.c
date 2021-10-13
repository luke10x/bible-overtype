#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <string.h>


#define PAIR_STATUS           1
#define PAIR_BOOK             2
#define PAIR_BOOK_SELECTED    3
#define PAIR_BOOK_HIGHLIGHT   4
#define PAIR_BOOK_DISABLED    5
#define PAIR_BOOK_SECTION     6
#define PAIR_SEARCH           7
#define PAIR_SEARCH_HIGHLIGHT 8
#define PAIR_SEARCH_SELECTED  9

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

// typedef struct mitem_t mitem_t; 

menu_t *menu_create(
  const mitem_t *all_items,
  unsigned short all_items_count,
  size_t item_size,
  unsigned short item_format_len
);


// #include "./menu.h"

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
  const mitem_t *all_items;      // This is the source of the data
  mitem_t *filtered_items;
  size_t item_size;
  unsigned short all_items_count; // How many unfiltered items there are
  unsigned short filtered_items_count; // How many left after filtering
  unsigned short item_format_len;    // Item label (number of multibyte chars + '\0')
  unsigned short selected_index;     // Currently selected item

  // dims
  unsigned short visible_columns;     // how many columns of items the window can fit
  unsigned short height;              // in how many rows the items will be arranged
  unsigned short delta;               // how many columns (or rows) skipped before starting to show
  unsigned short hpadding;               // space left free at the sides of the window
  unsigned short vpadding;               // space left free at the bottom and the top of the window

  // Should be extracted to canvas
  WINDOW *pad;
};

void menu_filter(menu_t *self, char *search) {
    mitem_t* selected_ptr = &self->filtered_items[self->selected_index * self->item_size];
    int selected_is_in_new_set = 0;
    self->filtered_items_count = 0; // Will be increased during this function

    char *item_label = malloc(self->item_format_len * sizeof(char));
    for (int i = 0; i < self->all_items_count; i++) {


        // TODO here it is now assuming is a book item,
        // but it also can be chapter item
        const bookinfo_t *book = &self->all_items[i * self->item_size].bookinfo;
        sprintf(
          item_label,
          "%2d. %-15s",
          book->id,
          book->title);

        // TODO separate string utils module
        char *found = strstr(strlwr(item_label),
                             strlwr(search));

        if (found != NULL) {
            self->filtered_items[self->filtered_items_count * self->item_size] = self->all_items[i];
            if (selected_ptr == &self->filtered_items[self->filtered_items_count * self->item_size]) {
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
unsigned short _get_pad_width(menu_t *self) {
    int columns = (int) (self->filtered_items_count / self->height) + 1;
    return columns * self->item_format_len;
}


// TODO pass winsz or better canvas as a param
void menu_recalculate_dims(menu_t *self, struct winsize winsz) {
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

    while (((int) (self->selected_index / self->height)) >= ((int) (self->delta + self->visible_columns))) {
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

void write_here(menu_t *self, const int row, const int col, int color_pair, char *str, struct winsize winsz)
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
void _draw_one_book(menu_t *self, int y, int x, bookinfo_t book, int key, struct winsize winsz)
{
    /* FIXME depends where this code will ter be moved,
     * it may have access to the global constant
     */
    int BOOK_FORMAT_LEN = self->item_format_len;

    char s[BOOK_FORMAT_LEN];
    sprintf(s, "%2d. %-15s", book.id, book.title);

    int color_pair = PAIR_BOOK;
    if (key == self->selected_index) {
        color_pair = PAIR_BOOK_SELECTED;
    }

    write_here(self, y, x * BOOK_FORMAT_LEN, color_pair, s, winsz);

    /*
    // Uncomment this later TODO (need to think how to pass along the search term)
    int pos = strpos(strlwr(s), strlwr(search));

    char *highlighted = malloc(strlen(search) + 1);
    highlighted[strlen(search)] = 0;
    memcpy(highlighted, s + pos, strlen(search));

    int color_pair_search = PAIR_SEARCH_HIGHLIGHT;
    if (key == selected) {
        color_pair_search = PAIR_SEARCH_SELECTED;
    }
    
    write_here(y, x * BOOK_FORMAT_LEN + pos, color_pair_search,
               highlighted);
    */
}

void menu_render(menu_t *self, struct winsize winsz) {
    // int columns = (int) (self->filtered_items_count / self->height) + 1;

    delwin(self->pad);

    self->pad = newpad(winsz.ws_row - 1, winsz.ws_col);
    scrollok(self->pad, 1);
    wclear(self->pad);

    int i = self->delta * self->height;
    for (int x = 0; x < self->visible_columns; x++) {
        for (int y = 0; y < self->height; y++) {
            if (i >= self->filtered_items_count)
                break;


            // Again book specific...
            const bookinfo_t book = self->all_items[i * self->item_size].bookinfo;
            _draw_one_book(self, y, x, book, i, winsz);

            i++;
        }
    }
}

void menu_handle_key(menu_t *self, char ch) {
    if (ch == 2) {
        if ((self->selected_index % self->height) < self->height - 1 && (self->selected_index + 1) < self->filtered_items_count) {
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
        if ((self->selected_index + self->height) < self->filtered_items_count) {
            self->selected_index += self->height;
        } else if ((self->filtered_items_count - (self->filtered_items_count % self->height) > self->selected_index)) {
            self->selected_index = self->filtered_items_count - 1;
        }
    }
}

int menu_get_selected_index(menu_t *self) {
    return self->selected_index;
}

int menu_get_delta(menu_t *self) {
    return self->delta;
}


menu_t *menu_create(
  const mitem_t *all_items,
  unsigned short all_items_count,
  size_t item_size,
  unsigned short item_format_len
) {


    bookinfo_t *boo = &all_items[0].bookinfo;
//   endwin(); 
  printf("Konstructor %d.\r\n", boo->chapters);
  
  

  menu_t *self = malloc(sizeof(menu_t));
  self->item_size = item_size;
  self->item_format_len = item_format_len;
  self->all_items = all_items;
//   printf("Konstructor2 %d.\r\n", self->all_items[6].bookinfo.chapters);
//   exit(7);

  self->filtered_items = malloc(all_items_count * item_size);
  self->selected_index = 0;

  menu_filter(self, NULL);

  return self;
}