#include "gex.h"
#include "dp.h"
#include "view_mode.h"
#include "file_handling.h"


// Global variables
appdef app = {.mode=VIEW_MODE};
status_windef status = {.win = NULL};
helper_windef helper = {.win = NULL};
hex_windef hex = {.win = NULL, .gc = NULL};
ascii_windef ascii = {.win = NULL, .gc = NULL};
char app_mode_desc[5][10] = {"Edit  ", "Insert", "Delete", "View  ", "Keys  "};



void initial_setup()
{
	// general app defaults
	app.mode=VIEW_MODE;	

	// set up for debug
	tmp = malloc(200);
//sprintf(tmp, "in degbug mode"); DP(tmp); 

	// Helper window 
	helper.helpmsg=malloc(40);
	strcpy(helper.helpmsg, "Last Key:");
	
	// Hex window
	hex.helpmsg=malloc(40);
	strcpy(hex.helpmsg, "Key:");
	hex.v_start = 0;
	//app.fsize = 1200;
	
}

void handlekeys(int k)
{
	// first check if we're Esc out of any non-view mode
	if (k==KEY_ESC) {
		// exit gracefully from non-view modes
		switch (app.mode){
	    	case EDIT_MODE:
	    		// call end edit mode routine
			break;
		case INSERT_MODE:
			// call end insert mode routine		
			break;
		case DELETE_MODE:
			// call end delete mode routine
			break;	
		default: break;
		}
		// set is back to view mode
		app.mode=VIEW_MODE;
		}
	
	// check if we've initiated a new mode
	switch(app.mode){
	case VIEW_MODE:
		switch(k){
		// special keys to change mode
		case 't': app.mode=TEST_KEYS; break;
		case 'e': app.mode=EDIT_MODE; break;
		case 'i': app.mode=INSERT_MODE; break;
		case 'd': app.mode=DELETE_MODE; break;
		case 'g': 
			goto_byte(); 	// popip
			move_view(k);	//force recalcs using move
			break;
		// otherwise check for movement keys
		default: 
			move_view(k); 
			break;
		}	
		break;
    	case EDIT_MODE:
		
		refresh_hex();
		break;
    	case INSERT_MODE:
	
		break;
    	case DELETE_MODE:
	
		break;
	case TEST_KEYS:

		if (k >= KEY_F(1) && k <= KEY_F(12)) {
			int n = k - KEY_F0;   // KEY_F(n) = KEY_F0 + n
			sprintf(helper.helpmsg, "F:%i  %o         ", n, k);
		} else {
			sprintf(helper.helpmsg, "Key: %i %o       ", k, k);
		}
		refresh_helper();
	
		break;
	default: 
		break;
	}

	update_all_window_contents();
	doupdate();
}


int main(int argc, char *argv[]) {
	// Debug panel on or off?
	DP_ON=true;
	
	// Initialize ncurses
	initscr();
	cbreak();		  // Line buffering disabled, Pass on everything
	noecho();		  // Don't echo input
	keypad(stdscr, true); // Enable function keys (like KEY_RESIZE)
	set_escdelay(50);

	// Initial app setup
	initial_setup();

	// open file
	if (open_file(argc, argv)) {
	
		// and go....
		create_windows();

		int ch;
		// Main loop to handle input
		ch = getch();
		while (	(app.mode != VIEW_MODE) ||
			((app.mode == VIEW_MODE) && (ch != 'q')))
			{
			if (ch == KEY_RESIZE) {
				// if not in view mode when the window is resized, then this could
				// cause issues, so simulate this by setting the character to Esc (27)
				// and calling handlekeys
				handlekeys(KEY_ESC);		
				// Re-create all windows on resize
				create_windows();
			} else {
				if (!app.too_small) handlekeys(ch);
			}
			ch = getch();
		}
	
		// no longer need the file open
		close_file();
	
	} else {
		DP("open file failed");
	}
		
	// Clean up and exit ncurses
	delete_windows();
	clear();
	refresh();
	endwin();
	free(tmp);
	return 0;
}
