#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "./scripture.h"

const bookinfo_t all_books[NUMBER_OF_BOOKS] = {
    {.id = 1,.title = "Genesis",.chapters = 50 },
    {.id = 2,.title = "Exodus",.chapters = 40 },
    {.id = 3,.title = "Leviticus",.chapters = 27 },
    {.id = 4,.title = "Numbers",.chapters = 36 },
    {.id = 5,.title = "Deuteronomy",.chapters = 34 },
    {.id = 6,.title = "Joshua",.chapters = 24 },
    {.id = 7,.title = "Judges",.chapters = 21 },
    {.id = 8,.title = "Ruth",.chapters = 4 },
    {.id = 9,.title = "1 Samuel",.chapters = 31 },
    {.id = 10,.title = "2 Samuel",.chapters = 24 },
    {.id = 11,.title = "1 Kings",.chapters = 22 },
    {.id = 12,.title = "2 Kings",.chapters = 25 },
    {.id = 13,.title = "1 Chronicles",.chapters = 29 },
    {.id = 14,.title = "2 Chronicles",.chapters = 36 },
    {.id = 15,.title = "Ezra",.chapters = 10 },
    {.id = 16,.title = "Nehemiah",.chapters = 13 },
    {.id = 17,.title = "Esther",.chapters = 10 },
    {.id = 18,.title = "Job",.chapters = 42 },
    {.id = 19,.title = "Psalms",.chapters = 150 },
    {.id = 20,.title = "Proverbs",.chapters = 31 },
    {.id = 21,.title = "Ecclesiastes",.chapters = 12 },
    {.id = 22,.title = "Song of Solomon",.chapters = 8 },
    {.id = 23,.title = "Isaiah",.chapters = 66 },
    {.id = 24,.title = "Jeremiah",.chapters = 52 },
    {.id = 25,.title = "Lamentations",.chapters = 5 },
    {.id = 26,.title = "Ezekial",.chapters = 48 },
    {.id = 27,.title = "Daniel",.chapters = 12 },
    {.id = 28,.title = "Hosea",.chapters = 14 },
    {.id = 29,.title = "Joel",.chapters = 3 },
    {.id = 30,.title = "Amos",.chapters = 9 },
    {.id = 31,.title = "Obadiah",.chapters = 1 },
    {.id = 32,.title = "Jonah",.chapters = 4 },
    {.id = 33,.title = "Micah",.chapters = 7 },
    {.id = 34,.title = "Nahum",.chapters = 3 },
    {.id = 35,.title = "Habakkuk",.chapters = 3 },
    {.id = 36,.title = "Zephaniah",.chapters = 3 },
    {.id = 37,.title = "Haggai",.chapters = 2 },
    {.id = 38,.title = "Zechariah",.chapters = 14 },
    {.id = 39,.title = "Malachi",.chapters = 4 },
    {.id = 40,.title = "Matthew",.chapters = 28 },
    {.id = 41,.title = "Mark",.chapters = 16 },
    {.id = 42,.title = "Luke",.chapters = 24 },
    {.id = 43,.title = "John",.chapters = 21 },
    {.id = 44,.title = "Acts",.chapters = 28 },
    {.id = 45,.title = "Romans",.chapters = 16 },
    {.id = 46,.title = "1 Corinthians",.chapters = 16 },
    {.id = 47,.title = "2 Corinthians",.chapters = 13 },
    {.id = 48,.title = "Galatians",.chapters = 6 },
    {.id = 49,.title = "Ephesians",.chapters = 6 },
    {.id = 50,.title = "Philippians",.chapters = 4 },
    {.id = 51,.title = "Colossians",.chapters = 4 },
    {.id = 52,.title = "1 Thessalonians",.chapters = 5 },
    {.id = 53,.title = "2 Thessalonians",.chapters = 3 },
    {.id = 54,.title = "1 Timothy",.chapters = 6 },
    {.id = 55,.title = "2 Timothy",.chapters = 4 },
    {.id = 56,.title = "Titus",.chapters = 3 },
    {.id = 57,.title = "Philemon",.chapters = 1 },
    {.id = 58,.title = "Hebrews",.chapters = 13 },
    {.id = 59,.title = "James",.chapters = 5 },
    {.id = 60,.title = "1 Peter",.chapters = 5 },
    {.id = 61,.title = "2 Peter",.chapters = 3 },
    {.id = 62,.title = "1 John",.chapters = 5 },
    {.id = 63,.title = "2 John",.chapters = 1 },
    {.id = 64,.title = "3 John",.chapters = 1 },
    {.id = 65,.title = "Jude",.chapters = 1 },
    {.id = 66,.title = "Revelation",.chapters = 22 },
};

const uint8_t all_chapters[MAX_CHAPTER] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
    71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
    91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
    101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
    111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
    121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
    131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
    141, 142, 143, 144, 145, 146, 147, 148, 149, 150
};

const bookinfo_t *get_all_books()
{
    const bookinfo_t *p = (const bookinfo_t *) &all_books;
    return p;
}

const chapter_t *get_this_many_chapters(uint8_t how_many)
{
    const chapter_t *p = (const chapter_t *) &all_chapters;
    return p;
}

const size_t get_chapter_blob(uint8_t ** blob, char *book_title,
                              chapter_t chapter)
{
    printf("You selected Book of %s chapter %d.\r\n", book_title, chapter);
    int absolute_number = 0;
    int i;
    const bookinfo_t *all_books = get_all_books();
    for (i = 0; strcmp(all_books[i].title, book_title) != 0; i++) {
        absolute_number += all_books[i].chapters;
    }
    absolute_number += chapter;
    printf("Which is total %dth chapter in whole Bible.\r\n", absolute_number);

    FILE *fp = fopen("usr/share/bible/chapter-index-kjv.bin", "r");
    printf("survived here:\r\n");
    fseek(fp, (absolute_number - 1) * 2 * sizeof(uint32_t), SEEK_SET);
    uint32_t bigend_start;
    uint32_t bigend_end;

    // The file is Big Endian
    fread(&bigend_start, sizeof(uint32_t), 1, fp);
    fread(&bigend_end, sizeof(uint32_t), 1, fp);
    fclose(fp);

    // On amd64 ntohl is no-op, but it does matter on arm
    uint32_t start = ntohl(bigend_start);
    uint32_t end = ntohl(bigend_end);

    printf("RAW Start: %u; End: %u;\r\n", bigend_start, bigend_end);
    printf("LOC Start: %u; End: %u;\r\n", start, end);

    size_t bloblen = (end - start); // * sizeof(uint8_t);
    
    char *p;
    p = *blob = malloc(bloblen * sizeof(uint8_t));
    
    FILE *fp_ = fopen("usr/share/bible/the-king-james-bible.txt", "r");
    fseek(fp_, start, SEEK_SET);
    // sprintf(*blob, "Good vibe!\r\n");
    fread(*blob, bloblen, 1, fp_);
    fclose(fp_);
    
    // Adding zero here will ensure the blob is always properly terminated
    // Interestingly it is always correct in WASM,
    // perhaps it is using a different way to fread
    p[bloblen] = 0;

    return bloblen;
}
