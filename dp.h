#ifndef GEX_DP_H
#define GEX_DP_H

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

void DP(const char *msg);

// so we show the panel?
bool DP_ON;
// makes debug panel usage easier
char *tmp;


#endif