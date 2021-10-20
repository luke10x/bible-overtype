#include <utf8proc.h>
#include <string.h>

#include "charlie.h"

char *fold2ascii(char *str)
{
    unsigned char *output;

    const size_t size_in_bytes = strlen(str);

    utf8proc_map((unsigned char *) str, size_in_bytes, &output,
                 UTF8PROC_DECOMPOSE | UTF8PROC_NULLTERM | UTF8PROC_STABLE |
                 UTF8PROC_STRIPMARK | UTF8PROC_CASEFOLD);

    return (char *) output;
}

int *is_mb_char_head(const ch) {
    int seventh = (ch >> 7) & 1;
    int sixth = (ch >> 6) & 1;

    return (seventh && sixth);
}

int *is_mb_char_tail(const ch) {
    int seventh = (ch >> 7) & 1;
    int sixth = (ch >> 6) & 1;

    return (seventh && sixth);
}

uint8_t *skip_n_unicode_chars_or_to_eol(int n, const char *source)
{

    char *p = *source;
    size_t len = 0;

    for (char *p = source; p; p++) {
      
        if (len == n) {
            return p;
        }
          if (!is_mb_char_tail((char)p[0])) {
            len++;
        }
    }
    return NULL;
}

typedef struct linebreaker_t {
    char *long_line;
    int *max_width;
    char *p;
    bool done;
};

linebreaker_t *lnbr_create(char *long_line, int max_width) {
    linebreaker_t *self = malloc(sizeof(linebreaker_t));

    self->max_width = max_width;
    self->long_line = long_line;
    self->p = long_line;
    self->done = 0;

    return self;
}

char *lnbr_take_some(linebreaker_t *self) {

    if (self->p == NULL) {
        return NULL;
    }
    if (self->done) {
        return NULL;
    }

    char *p1 = self->p;

    int count = 0;
    while (self->p) {

        char ch = (char) self->p[0];
        if (ch == 0) {
            self->done = 1;
            break;
        }
        if (!is_mb_char_tail(ch)) {
            count++;
        }
        if (count  == self->max_width) {
            self->p++;

            if (is_mb_char_head(ch)) {
                while (is_mb_char_tail((char) self->p[1])) self->p++;
            }
            break;
        }
        self->p++;
    }

    int piece_len = self->p - p1;

    char *piece = malloc(piece_len + 1);

    strncpy(piece, p1, piece_len);
    piece[piece_len] = 0;

    return piece;
}
