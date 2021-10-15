#include <utf8proc.h>
#include <string.h>

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