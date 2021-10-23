#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * get_stream_blob(FILE *fp)
{
    int block_size = 10;
    char *buf = malloc(block_size);
    int read_blocks = 0;
    int total_blocks = 0;
    int read_bytes = 0;
    int total_bytes = 0;
    char *all = malloc(1);

        // endwin(); 
    while((read_blocks = fread(buf, block_size, 1, fp))) {
        total_blocks += read_blocks;
        size_t read_bytes = block_size * read_blocks;
        size_t total_bytes = block_size * total_blocks;
        all = (char *) realloc(all, total_bytes);
        // printf("all resized to %d bytes\r\n", total_bytes);
        memcpy(all + total_bytes - read_bytes, buf, read_bytes);
        // printf("copy %d bytes from '%s' to all[%d]\r\n", read_bytes, buf, total_bytes - read_bytes);
        // endwin(); printf("hudnt (%s) %d + %d\r\n", buf, total_bytes, read_bytes); exit(0);
    }
    
    return all;
    // printf("hudnt ''%s' (%s) %d + %d\r\n", *all, buf, total_bytes, read_bytes); exit(0);

}
