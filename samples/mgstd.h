#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <panel.h>

#define FALSE 0
#define TRUE 1

#define DEBUG 1

#if DEBUG
    #define DD(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
    #define DD(fmt, ...)  // nothing
#endif

