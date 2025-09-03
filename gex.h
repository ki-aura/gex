#ifndef GEX_MAIN_H
#define GEX_MAIN_H

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
#include <menu.h>
#include "dp.h"


// keys we need that aren't already defined by ncurses
#define KEY_ESCAPE 27
#define KEY_MAC_ENTER 10	// KEY_ENTER already defined as send key for terminal
#define KEY_TAB 9
#define KEY_SPACE 32


// types of popup question
typedef enum { 	
	PTYPE_YN,
	PTYPE_CONTINUE,
} popup_types;


typedef enum {
    EDIT_MODE, 
    INSERT_MODE,
    DELETE_MODE,
    VIEW_MODE, 
} app_mode;

// Overall (non-window) screen attributes & app status
typedef struct {
	// stdscr details
	int cols;
	int rows;
	bool too_small; 	// less than 2 hex rows and 16 hex chars wide
	// general 
	bool in_hex;		// track which pane we're in during edit
	app_mode mode;		// view, edit etc
	// file & mem handling
	size_t fsize;		// file size
	char *fname;		// file name
	unsigned char *map;  	// mmap base
	int fd;			// file descriptor
	struct stat fs;		// file stat
} appdef;

// Window definitions
typedef struct {
	WINDOW *win;
	int height;	// grid height including border
	int width;	// grid width including border
	int digits;	// grid width in hex digits (i.e. 3 chars / digit)
	int rows;	// grid height excluding border
	int grid;	// grid size in total hex digits (portion of file)
	// file handling and viewing
	unsigned long v_start;	// file offset location of start of grid
	unsigned long v_end;	// file location of end of grid
	char *gc;	// viewable grid contents
	// buffers for edit and cursor tracker variables
	char *gc_copy;	// copy of viewable grid contents
	unsigned char *map_copy; 	// copy of map that relates to the screen
	int map_copy_len;
	int max_row;	// this is the max row we can edit if screen > file size
	int max_col; 	// this is the max col on the max row we can edit if screen > file size
	int max_digit; // this is the max digit on the max row we can edit if screen > file size
	int cur_row;
	int cur_col;
	int cur_digit;	// which hex digit within the col
	bool is_lnib;	// are we on the left nibble
	bool changes_made;	// have we changed anything?
} hex_windef;

typedef struct {
	WINDOW *win;
	int height;
	int width;
	char *gc;
	// don't need digits, grid or v_start/v_end as same as hex
	// buffers for edit
	char *gc_copy;
	// don't need cur_row or cur_col as same as hex.row and hex.digits
} ascii_windef;

typedef struct {
	int height;
	int width;
	WINDOW *win;
} status_windef;

typedef struct {
	int height;
	int width;
	WINDOW *win;
} helper_windef;


// helper functions
bool popup_question(char *qline1, char *qline2, popup_types pt, app_mode mode);
void byte_to_hex(unsigned char b, char *out);
char byte_to_ascii(unsigned char b);
int hex_char_to_value(char c);
unsigned char hex_to_byte(char high, char low);
// main loop
void handle_global_keys(int k);
void initial_setup();
void final_close();
void refresh_helper(char *helpmsg);
void refresh_status();
// testing...
void create_view_menu(WINDOW *status_win);



extern appdef app;
extern status_windef status;
extern helper_windef helper;
extern hex_windef hex;
extern ascii_windef ascii;
extern char app_mode_desc[][10];

#endif
