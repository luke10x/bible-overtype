#ifndef status_h__

#define status_h__

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

// These also need to be repeaded in the implementation file
typedef struct status_t status_t;

struct status_t {
    char *msg;
    char *search_term;
    unsigned short width;
    struct winsize winsz;
};

status_t *status_create();

void status_set_msg(status_t *self, int has_cursor);

void status_render(status_t *self, struct winsize winsz);

void status_handle_key(status_t *self, char ch);

char *status_get_search_term(status_t *self);

#endif
