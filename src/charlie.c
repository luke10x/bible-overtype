#include <utf8proc.h>
#include <string.h>

char *fold2ascii(char *str) {
    unsigned char *output;

    const size_t size_in_bytes = strlen(str);

    utf8proc_map((unsigned char *) str, size_in_bytes, &output,
                 UTF8PROC_DECOMPOSE | UTF8PROC_NULLTERM | UTF8PROC_STABLE |
                 UTF8PROC_STRIPMARK | UTF8PROC_CASEFOLD);

    return (char *)output;
}