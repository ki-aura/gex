#ifndef GEX_MAIN_H
#define GEX_MAIN_H

#include <sys/stat.h> // Provides functions for retrieving and manipulating file status (e.g., stat, fstat)
//#include <glob.h>     // Provides pattern matching for filenames (pathname expansion)
//#include <regex.h>    // Provides functions for regular expression matching
#include <assert.h>   // Provides the assert macro for debugging and checking invariant conditions
#include <ctype.h>    // Provides functions for character classification (e.g., isalpha, isdigit) and conversion
#include <errno.h>    // Defines macros for reporting error conditions (e.g., errno, EACCES)
#include <fcntl.h>    // Provides file control functions (e.g., open, creat, file status flags)
#include <limits.h>   // Defines characteristics of integral types (e.g., INT_MAX, CHAR_BIT)
#include <ncurses.h>  // Provides functions for terminal-independent screen-handling and text-based UIs
#include <panel.h>    // Provides functions for stacking and manipulating ncurses windows as panels
#include <signal.h>   // Provides functions and constants for signal handling (e.g., kill, raise)
#include <stdarg.h>   // Provides support for functions with variable numbers of arguments (variadic functions)
#include <stdbool.h>  // Defines the boolean type bool and the macros true and false
#include <stdint.h>   // Defines exact-width integer types (e.g., int32_t, uint64_t)
#include <stdio.h>    // Provides standard input/output functions (e.g., printf, scanf, file I/O)
#include <stdlib.h>   // Provides general utilities (e.g., memory allocation, random numbers, process control)
#include <string.h>   // Provides functions for manipulating strings and memory blocks (e.g., strcpy, memcpy)
#include <sys/mman.h> // Provides memory management declarations (e.g., mmap, munmap)
#include <unistd.h>   // Provides access to POSIX operating system API (e.g., fork, exec, read, close)


#include "rbtree.h"

#define GEX_VERSION "2.2.3"

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
int final_close(void);
clickwin get_window_click(int *row, int *col);


extern appdef app;
extern status_windef status;
extern hex_windef hex;
extern ascii_windef ascii;
extern char *tmp;// makes debug panel usage easier
extern MEVENT event;

extern struct FByte search;
extern struct FByte *found, *nod;

// this needs to be last as it relies on the typedefs above

#include "gex_helper_funcs.h"
#include "keyb_man.h"
#include "win_man.h"
#include "file_handling.h"


#endif
