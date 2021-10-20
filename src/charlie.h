#ifndef charlie_h__

#define charlie_h__

char *fold2ascii(char *str);

uint8_t *skip_n_unicode_chars_or_to_eol(int n, const char *source);

typedef struct linebreaker_t linebreaker_t;

linebreaker_t *lnbr_create(char *long_line, int max_width);

char *lnbr_take_some(linebreaker_t *linebreaker);

#endif
