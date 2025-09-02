#ifndef GEX_FILE_HANDLING_H
#define GEX_FILE_HANDLING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <panel.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

bool open_file(int argc, char *argv[]);
void close_file();


#endif
