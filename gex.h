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

// keys we need that aren't already defined by ncurses
#define KEY_ESC 27

// template functions
void handlekeys(int k);
void initial_setup();

// Overall (non-window) screen attributes & app status
typedef enum {
	// first modes that need a visible description
    EDIT_MODE, 
    INSERT_MODE,
    DELETE_MODE,
    VIEW_MODE, 
    TEST_KEYS,
} app_mode;

typedef struct {
	int cols;
	int rows;
	bool too_small; 	// less than 2 hex rows and 16 hex chars wide
	app_mode mode;
	size_t fsize;		// file size
	char *fname;		// file name
	unsigned char *map;  	// mmap base
	int fd;			// file descriptor
	struct stat fs;		// file stat
	bool hex_pane_active;	// track which pane we're in
} appdef;


// Window definitions
typedef struct {
	int height;
	int width;
	WINDOW *win;
} status_windef;

typedef struct {
	int height;
	int width;
	WINDOW *win;
	char *helpmsg;
} helper_windef;

typedef struct {
	WINDOW *win;
	char *helpmsg;	
	int height;	// grid height including border
	int width;	// grid width including border
	int digits;	// grid width in hex digits (i.e. 3 chars / digit)
	int rows;	// grid height
	int grid;	// grid size in total hex digits (portion of file)
	unsigned long v_start;	// file offset location of start of grid
	unsigned long v_end;	// file location of end of grid
	char *gc;	// grid contents
} hex_windef;

typedef struct {
	int height;
	int width;
	WINDOW *win;
	char *gc;
} ascii_windef;

extern appdef app;
extern status_windef status;
extern helper_windef helper;
extern hex_windef hex;
extern ascii_windef ascii;
extern char app_mode_desc[][10];

#endif
