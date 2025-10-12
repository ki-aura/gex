#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // For strncpy, strcmp, strrchr, memset, memcpy, snprintf
#include "memsafe.h"


// ------------------Memory safe allocation helpers (Wrapper Functions) ----------
// Wrappers around standard memory allocation functions (malloc/calloc/realloc)
// that perform error checking and exit the program on failure.
// This is a common pattern for robust C utilities.
void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL && size>0) {
        fprintf(stderr, "Fatal: Out of memory (malloc %zu bytes).\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (ptr == NULL && count>0 && size>0) {
        fprintf(stderr, "Fatal: Out of memory (calloc %zu count %zu bytes).\n", count, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL && size > 0) {
        fprintf(stderr, "Fatal: Out of memory (realloc %zu bytes).\n", size);
        free(ptr);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

