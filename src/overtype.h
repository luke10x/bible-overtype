#ifndef overtype_h__
#define overtype_h__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>


// These also need to be repeaded in the implementation file
typedef struct overtype_t overtype_t;

overtype_t *ovt_create(uint8_t *blob, char *title);

int ovt_handle_key(overtype_t *self, char ch);

char ovt_try_autotext(overtype_t *self, char ch);

void ovt_recalculate_size(overtype_t *self, struct winsize winsz);

void ovt_render(overtype_t *self, struct winsize winsz);

int ovt_is_done(overtype_t *self, char *print_this_if_done);

#endif
