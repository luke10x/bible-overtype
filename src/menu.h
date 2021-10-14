#ifndef menu_h__

#define menu_h__

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

// These also need to be repeaded in the implementation file
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

menu_t *menu_create(const mitem_t * all_items,
                    unsigned short all_items_count,
                    size_t item_size, unsigned short item_format_len);

void menu_filter(menu_t * self, char *search);

void menu_recalculate_dims(menu_t * self, struct winsize winsz);

void menu_render(menu_t * self, struct winsize winsz);

void menu_fast_render(menu_t * self, int old_selected_index,
                      struct winsize winsz);

void menu_handle_key(menu_t * self, char ch);

mitem_t *menu_get_selected_item();

unsigned short menu_get_selected_index(menu_t * self);

unsigned short menu_get_delta(menu_t * self);

// returns whether search changed and neeeds to be resized
unsigned short menu_get_filtered_item_count(menu_t * self);

unsigned short menu_is_done(menu_t * self);

void menu_finalize(menu_t * self);

#endif
