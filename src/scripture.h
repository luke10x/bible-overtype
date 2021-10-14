#ifndef scripture_h__

#include  <stdio.h>

#define scripture_h__

#define NUMBER_OF_BOOKS       66
#define MAX_CHAPTER          150

struct bookinfo_t {
    unsigned short int id;
    unsigned short int
     chapters;
    char *title;
};

typedef struct bookinfo_t bookinfo_t;

typedef uint8_t chapter_t;

const bookinfo_t *get_all_books();

const uint8_t *get_this_many_chapters(uint8_t how_many);

const size_t get_chapter_blob(uint8_t **blob, char *book_title, chapter_t chapter);

#endif
