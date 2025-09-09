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
#include <menu.h>
#include <assert.h>
#include "khash.h"


// keys we need that aren't already defined by ncurses
#define KEY_ESCAPE 27
#define KEY_MAC_ENTER 10	// KEY_ENTER already defined as send key for terminal
#define KEY_TAB 9
#define KEY_SPACE 32
#define KEY_MAC_DELETE 127
#define KEY_LEFT_PROXY 222
#define KEY_NCURSES_BACKSPACE KEY_BACKSPACE

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


// Initialize khash: signed long integer key  → slots holding unsigned char
KHASH_MAP_INIT_INT64(charmap, unsigned char)
/*
Function / Macro	Description
KHASH_MAP_INIT_INT(name, valtype)	Defines a complete integer-keyed hash table type 
					name with values of type valtype and all related macros/functions.
khash_t(name) *h	Declares a pointer h to a hash table of the type given name.
h = kh_init(name)	Allocate and initialize a new hash table; returns a pointer to the table
kh_destroy(name, h)	Free memory used by hash table h.

khiter_t k 		integer index to a slot within a table. Returned by functions that 
			modify the table or iterate / check on key existence Remember
			** SLOTS ARE PRE-ALLOCATED INTERNALLY AND ARE ALL INITIALLY EMPTY **
			
kh_put(name, h, key, &ret)	Insert new key (or get existing) and return its slot; 
				ret indicates if new (1), existing (0), or failure (-1).
				**NOTE nothing is overwritten at this point, you just have a 
				slot index and a return code that says if it's new or existing
kh_val(h, k)		Access the value at slot k. used to set or retrieve the value
kh_key(h, k)		read only lookup of a key; returns the slot index k. can NOT be used to update the key
kh_get(name, h, key)	read only Look up key; returns slot index or kh_end(h) if not found but 
			doesn't create one if it does exist (compare this to kh_put)
kh_del(name, h, k)	Delete the key/value at slot k.
kh_begin(h)		Returns the index of the first slot in the table (all slots may be empty).
kh_end(h)		Returns the index just past the last slot. (ie used as iter loop terminator)
kh_exist(h, k)		Returns true if slot k contains a valid key/value.
kh_size(h)		Returns the number of valid elements currently in the table.
kh_clear(name, h)	Remove all elements but keep table allocated (reset all slots to empty).

** NOTE FOR UNSIGNED LONGS **
KHASH_MAP_INIT_INT64(name, valtype) 	will create a type long enough to take unsigned longs, but
					INT64 is signed. this isn't a problem as KH doesn't care if 
					your key is negative, so casting from UL to INT64 isn't a
					problem. other funcs will need to look like this:
kh_get(name, h, (int64_t)UL_key)	casting the ul key to an int64
*/


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
	// hash table for file updates
	int lastkey; 	// debug use
	khash_t(charmap) *edmap;
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

typedef struct {
	int height;
	int width;
	WINDOW *border;
	WINDOW *win;
	char *helpmsg;
} helper_windef;


void handle_global_keys(int k);
bool initial_setup(int argc, char *argv[]);
void final_close(int signum);
clickwin get_window_click(MEVENT *event, int *row, int *col);


extern appdef app;
extern status_windef status;
extern helper_windef helper;
extern hex_windef hex;
extern ascii_windef ascii;
extern char *tmp;// makes debug panel usage easier
extern khiter_t slot; // general hash usage
extern int khret;
extern MEVENT event;

// snprintf(tmp, 200, "msg %lu %d", app.fsize , hex.grid); DP(tmp); 


void create_view_menu(WINDOW *status_win);


// this needs to be last as it relies on the typedefs above
#include "gex_helper_funcs.h"
#include "edit_mode.h"
#include "view_mode.h"
#include "keyb_man.h"
#include "win_man.h"

#endif
