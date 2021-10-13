#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <string.h>

typedef struct status_t status_t;

struct status_t {
    char *msg;
    char *search_term;
    struct winsize winsz;
    unsigned short has_cursor;
};

status_t *status_create() {
    status_t *self = malloc(sizeof(status_t));

    self->search_term = "";
    return self;
}

void status_set_msg(status_t * self, char *msg, int has_cursor)
{
  self->msg = msg;
}

void status_render(status_t * self, struct winsize winsz) {

}

void status_handle_key(status_t * self, char ch) {
    if (ch > '0' && ch < 'z') {
        mvaddch(8, 1, ch);
    }
}

char *status_get_search_term(status_t * self) {
  return self->search_term;
}
