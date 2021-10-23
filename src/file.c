#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_stream_blob(FILE * fp)
{ 
    int memory = 10;
    char *all = malloc(10);
    char ch;

    int i = 0;

    while((ch = fgetc(fp)) != EOF) {
        if (i == memory - 1) {
            memory += 10;
            all = (char *) realloc(all, memory);
        }
        all[i] = ch;
        i++;
    }

    return all;
    // printf("hudnt ''%s' (%s) %d + %d\r\n", *all, buf, total_bytes, read_bytes); exit(0);

}
