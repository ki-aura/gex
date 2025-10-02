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
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <signal.h>
#include <assert.h>
#include "rbtree.h"

#define GEX_VERSION "1.5.0"

// keys we need that aren't already defined by ncurses
#define KEY_ESCAPE 27
#define KEY_MAC_ENTER 10	// KEY_ENTER already defined as send key for terminal
#define KEY_TAB 9
#define KEY_SPACE 32
#define KEY_MAC_DELETE 127
#define KEY_OTHER_DELETE 8		// e.g. on debian 
#define KEY_LEFT_PROXY 222
#define KEY_NCURSES_BACKSPACE KEY_BACKSPACE // this is 263

// types of popup question
typedef enum { 	
	PTYPE_YN,
	PTYPE_CONTINUE,
	PTYPE_UNSIGNED_LONG,
} popup_types;

typedef enum {
	WIN_HEX,
	WIN_ASCII,
	WIN_OTHER,
} clickwin;


// Overall (non-window) screen attributes & app status
typedef struct {
	// stdscr details
	int cols;
	int rows;
	bool too_small; 	// less than 2 hex rows and 16 hex chars wide
	// general 
	bool in_hex;		// track which pane we're in during edit
	// file & mem handling
	size_t fsize;		// file size
	char *fname;		// file name
	unsigned char *map;  	// mmap base
	int fd;			// file descriptor
	struct stat fs;		// file stat
	int lastkey; 		// debug : last actual OR simulated key
	int lasteditkey;	// debug : last key used in a byte edit
} appdef;


// Window definitions
typedef struct {
	WINDOW *border;
	WINDOW *win;
	int height;	// grid height excluding border
	int width;	// grid width excluding border
	int grid;	// grid size in total hex / ascii digits (portion of file)
	// file handling and viewing
	unsigned long v_start;	// file offset location of start of grid
	unsigned long v_end;	// file location of end of grid

	int map_copy_len;
	int max_row;	// this is the max row we can edit if screen > file size
	int max_col; 	// this is the max col on the max row we can edit if screen > file size
	int max_digit; // this is the max digit on the max row we can edit if screen > file size
	int cur_row;	// cursor location (i.e. where to show it rather than where it is)
	int cur_col;
	int cur_digit;	// which hex digit (hinib lownib space) the cursor is on
	bool is_hinib;	// are we on the hi (left) nibble
} hex_windef;

typedef struct {
	WINDOW *border;
	WINDOW *win;
	int height;
	int width;
} ascii_windef;

typedef struct {
	int height;
	int width;
	WINDOW *border;
	WINDOW *win;
} status_windef;


void handle_global_keys(int k);
bool initial_setup(int argc, char *argv[]);
void final_close(int signum);
clickwin get_window_click(int *row, int *col);
bool create_main_menu(void);


extern appdef app;
extern status_windef status;
extern hex_windef hex;
extern ascii_windef ascii;
extern char *tmp;// makes debug panel usage easier
extern MEVENT event;

extern struct FByte search;
extern struct FByte *found, *nod;


// snprintf(tmp, 200, "msg %lu %d", app.fsize , hex.grid); DP(tmp); 




// this needs to be last as it relies on the typedefs above
#include "gex_helper_funcs.h"
#include "keyb_man.h"
#include "win_man.h"
#include "file_handling.h"

#endif
