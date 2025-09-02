#ifndef GEX_VIEW_MODE_H
#define GEX_VIEW_MODE_H

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

void handle_view_keys(int k);
void byte_to_hex(unsigned char b, char *out);
char byte_to_ascii(unsigned char b);
int hex_char_to_value(char c);
unsigned char hex_to_byte(char high, char low);
void populate_grids();
void refresh_ascii();
void refresh_hex();
void size_windows();
void update_all_window_contents();
void create_windows();
void delete_windows();
void goto_byte();


/*
	case KEY_DOWN:
		break;
	case KEY_UP:
		break;
	case KEY_LEFT:
		break;
	case KEY_RIGHT:
		break;
	case KEY_HOME:
		break;
	case KEY_BACKSPACE:
		break;
	case KEY_F0:
		break;
	case KEY_F(n):
		break;
	case KEY_SF:
		break;
	case KEY_SR:
		break;
	case KEY_NPAGE:
		break;
	case KEY_PPAGE:
		break;
	case KEY_BTAB:
		break;
	case KEY_END:
		break;
	case KEY_SEND:
		break;
	case KEY_SHOME:
		break;
	case KEY_SLEFT:
		break;
	case KEY_SRIGHT:
		break;
	case KEY_MOUSE:
		break;
	case KEY_RESIZE:
		break;
	case KEY_ESC:
		break;

*/
//decimals
//10 enter
//27 esc
//9 tab
//32 space

#endif
