#ifndef GEX_EDIT_MODE_H
#define GEX_EDIT_MODE_H

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
#include <menu.h>

void handle_edit_keys(int k);
void init_edit_mode();
void end_edit_mode();

#endif
