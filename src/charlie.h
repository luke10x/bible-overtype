#ifndef charlie_h__

#define charlie_h__

char *fold2ascii(char *str);

char *skip_n_unicode_chars_or_to_eol(int n, char *source);

typedef struct linebreaker_t linebreaker_t;

linebreaker_t *lnbr_create(char *long_line, int max_width);

char *lnbr_take_some(linebreaker_t *linebreaker);

char *get_utf_char_at_from_string(int pos, char *str);

#endif
