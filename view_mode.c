#include "gex.h"
#include "view_mode.h"


void v_handle_keys(int k)
{
//////DP("in move mode");
////DP_ON = false;

	switch(k){
	// simple cases for home and end
	case KEY_HOME:
		////DP("home");
		hex.v_start = 0;
		;break;
	case KEY_END:
		//DP("end");
		hex.v_start = app.fsize - hex.grid ;
		break;
	// going down file, add here an rely on boundary conditions to check of we're over
	case KEY_RIGHT:
		//DP("down l char");
		hex.v_start++;
		break;
	case KEY_DOWN:
		hex.v_start += ascii.width;
		break;
	case KEY_NPAGE:
		hex.v_start += hex.grid;
		break;
	// going up the file, we can't check of < 0 because we're working with unsigned ints
	// so we have to ensure we don't go below zero and end up massive
	case KEY_LEFT:
	// up on char
		if (hex.v_start > 0) 
			hex.v_start--;
		break;
	case KEY_UP:
	// up one line
		if(hex.v_start > (unsigned long)ascii.width)
			hex.v_start -= ascii.width;
		else
			hex.v_start = 0;
		break;
	// up one full grid
	case KEY_PPAGE:
		if(hex.v_start > (unsigned long)hex.grid)
			hex.v_start -= hex.grid;
		else
			hex.v_start = 0;
		break;
	}

	// boundary conditions
	if ((hex.v_start >= app.fsize) ||
			((hex.v_start + hex.grid) >= app.fsize))
		hex.v_start = app.fsize - hex.grid;
	
	if (app.fsize <= (unsigned long)hex.grid) 
		hex.v_start = 0;
	
	// calc v_end
	hex.v_end = hex.v_start + hex.grid -1;
	if (hex.v_end >= app.fsize) hex.v_end = app.fsize -1;
}

void v_populate_grids()
{
/*	if (!app.too_small){
		char t_hex[2];
		for(int i=0; (hex.v_start + i) <= hex.v_end; i++){
			byte_to_hex(app.map[hex.v_start + i], t_hex);
			
			app.map[hex.v_start + (i * 3)] = t_hex[0];
			app.map[hex.v_start + (i * 3) +1] = t_hex[1];		
			
			ascii.gc[i] = byte_to_ascii(app.map[hex.v_start + i]);
		}
	}
*/
}


void v_refresh_hex()
{
		char t_hex[2];
	box(hex.border, 0, 0);

	int hr=0; // offset print row on grid. 1 avoids the borders
	int hc=0;
	int i=0; 
	int grid_points = hex.grid * 3;
	while(i<grid_points ){
		// print as much as a row as we can
		while ((i < grid_points) && (hc < hex.width)){
			byte_to_hex(app.map[hex.v_start + i], t_hex);			
			mvwprintw(hex.win, hr, hc, "%c", t_hex[0]);
			mvwprintw(hex.win, hr, hc+1, "%c", t_hex[1]);

			hc+=3;
			i++;
		}
		hc = 0;
		hr++;
	}
	wnoutrefresh(hex.border);
	wnoutrefresh(hex.win);
}

void v_refresh_ascii()
{
	box(ascii.border, 0, 0);
	
	int hr=0; // offset print row on grid. 1 avoids the borders
	int hc=0;
	int i=0; 
	while(i<hex.grid){
		// print as much as a row as we can
		while ((i < hex.grid) && (hc < ascii.width)){
			mvwprintw(ascii.win, hr, hc, "%c", byte_to_ascii(app.map[hex.v_start + i]));
			hc++;
			i++;
		}
		hc = 0;
		hr++;
	}
	wnoutrefresh(ascii.border);
	wnoutrefresh(ascii.win);
}

void v_update_all_windows()
{
	if (!app.too_small){
		v_populate_grids();
	
		refresh_helper("Options: quit insert edit delete test goto");
		refresh_status();
		v_refresh_hex();
		v_refresh_ascii();
		doupdate();
	}
}

void size_windows()
{
	// Get the current screen dimensions
	getmaxyx(stdscr, app.rows, app.cols);
	app.too_small = false;
	
	// Calculate window heights 
	status.height = 2; 
	helper.height = 2; 
	hex.height = app.rows - status.height - helper.height - 6; // 4 for the borders
	ascii.height = hex.height;					
	
	// calculate window widths
	hex.width = ((int)((app.cols-4) / 4) * 3.0); 
	ascii.width = (int)(hex.width / 3);
	status.width = hex.width + ascii.width + 2; // 2 for the internal borders of hex & ascii
	helper.width = status.width;

	hex.grid = ascii.width * ascii.height;
	
	// Check for minimum screen size to prevent crashes
	if (app.rows < (status.height + helper.height + 10) || app.cols < 68) {
		// If the screen is too small, just display a message and return
		app.too_small = true;
		return;
	}
}

// Function to create and resize all windows
void create_windows() 
{
   // get sizes
   size_windows();
 
   // First, clear the entire screen to get rid of any artifacts
	delete_windows();
	clear();
	refresh();

  // Check for minimum screen size to prevent crashes
	if (app.too_small) {
		// If the screen is too small, just display a message and return
		mvprintw(app.rows / 2, (app.cols - 45) / 2, "Screen is too small. Please resize to continue.");
		refresh(); // refresh becuase i'm writing to main screen
	} else 	{
		// Create new windows & borders based on the new dimensions
		// newwin(num_rows, num_cols, start_row, start_col)
		status.border = newwin(status.height+2, status.width+2, 0, 0);
		hex.border = newwin(hex.height+2, hex.width+2, status.height+2, 0);
		ascii.border = newwin(ascii.height+2, ascii.width+2, status.height+2, hex.width+2);
		helper.border = newwin(helper.height+2, helper.width+2, status.height+hex.height+4, 0);
			
		// Draw borders and content for each window
		box(status.border, 0, 0);
		box(hex.border, 0,0);
		box(ascii.border, 0, 0);
		box(helper.border, 0, 0);
		
		status.win = newwin(status.height, status.width, 1, 1);
		hex.win = newwin(hex.height, hex.width, status.height+3, 1);
		ascii.win = newwin(ascii.height, ascii.width, status.height+3,(hex.width+3));
		helper.win = newwin(helper.height, helper.width, status.height+hex.height+5, 1);
	}
	
	// simulate a move so that the changes in the grid size are reflected 
	v_handle_keys(KEY_ESCAPE);
	
	// populate the windows
	v_update_all_windows();
}

// Function to delete all windows
void delete_windows() 
{
if (status.win != NULL) {delwin(status.win); status.win = NULL; }
if (helper.win != NULL) {delwin(helper.win); helper.win = NULL; }
if (hex.win != NULL) {delwin(hex.win); hex.win = NULL; }
if (ascii.win != NULL) {delwin(ascii.win); ascii.win = NULL; } 
if (status.border != NULL) {delwin(status.border); status.border = NULL; }
if (helper.border != NULL) {delwin(helper.border); helper.border = NULL; }
if (hex.border != NULL) {delwin(hex.border); hex.border = NULL; }
if (ascii.border != NULL) {delwin(ascii.border); ascii.border = NULL; } 
}

void v_goto_byte() 
{
	snprintf(tmp, 60, "Goto Byte? (0-%lu)", (unsigned long)app.fsize);
	
	// hex.v_start = will either be a new valid value or 0
	hex.v_start = popup_question(tmp, "", PTYPE_UNSIGNED_LONG);
}


