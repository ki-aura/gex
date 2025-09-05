#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uthash.h"

typedef struct {
    int pos;            // position in sentence
    char ch;            // character
    UT_hash_handle hh;  // uthash handle
} letter_t;

// Sort by ASCII value
int cmp_ascii(letter_t *a, letter_t *b) {
    return (int)a->ch - (int)b->ch;
}

// Sort by position
int cmp_pos(letter_t *a, letter_t *b) {
    return a->pos - b->pos;
}

int main() {
    const char *sentence = "Mary had a little lamb it's fleece was white as snow";
    int len = strlen(sentence);

    letter_t *letters = NULL; // uthash table
    for (int i = 0; i < len; i++) {
        letter_t *item = malloc(sizeof(letter_t));
        item->pos = i;
        item->ch = sentence[i];
        HASH_ADD_INT(letters, pos, item);
    }

    // 1. Sort by ASCII
    printf("Sorted by ASCII:\n");
    HASH_SORT(letters, cmp_ascii);
    for (letter_t *s = letters; s != NULL; s = s->hh.next) {
        printf("%c", s->ch);
    }
    printf("\n\n");

    // 2. Sort by position
    printf("Sorted by position:\n");
    HASH_SORT(letters, cmp_pos);
    for (letter_t *s = letters; s != NULL; s = s->hh.next) {
        printf("%c", s->ch);
    }
    printf("\n\n");

    // 3. Every other letter (after sorting by pos again)
    printf("Every other letter (by pos):\n");
    int toggle = 0;
    for (letter_t *s = letters; s != NULL; s = s->hh.next) {
        if (toggle) {
            printf("%c", s->ch);
        }
        toggle = !toggle;
    }
    printf("\n");

    // Free memory
    letter_t *cur, *tmp;
    HASH_ITER(hh, letters, cur, tmp) {
        HASH_DEL(letters, cur);
        free(cur);
    }

    return 0;
}
