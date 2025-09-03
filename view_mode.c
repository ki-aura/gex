#include "gex.h"
#include "view_mode.h"


void v_handle_keys(int k)
{
//DP("in move mode");
DP_ON = false;

	switch(k){
	// simple cases for home and end
	case KEY_HOME:
		DP("home");
		hex.v_start = 0;
		;break;
	case KEY_END:
		DP("end");
		hex.v_start = app.fsize - hex.grid ;
		break;
	// going down file, add here an rely on boundary conditions to check of we're over
	case KEY_RIGHT:
		DP("down l char");
		hex.v_start++;
		break;
	case KEY_DOWN:
		hex.v_start += hex.digits;
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
		if(hex.v_start > (unsigned long)hex.digits)
			hex.v_start -= hex.digits;
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
	free(hex.gc);
	free(ascii.gc);
	
	hex.gc = malloc((hex.grid * 3) + 1);
	ascii.gc = malloc(hex.grid + 1);

	memset(hex.gc, ' ', (hex.grid * 3));
	memset(ascii.gc, ' ', (hex.grid));
	
	char t_hex[2];
	for(int i=0; (hex.v_start + i) <= hex.v_end; i++){
		byte_to_hex(app.map[hex.v_start + i], t_hex);
		
		hex.gc[i * 3] = t_hex[0];
		hex.gc[(i * 3) +1] = t_hex[1];		
		
		ascii.gc[i] = byte_to_ascii(app.map[hex.v_start + i]);
	}
}


void v_refresh_hex()
{
	box(hex.win, 0, 0);

	int hr=1; // offset print row on grid. 1 avoids the borders
	int hc=1;
	int i=0; 
	int grid_points = hex.grid * 3;
	int row_points = hex.digits * 3;
	while(i<grid_points ){
		// print as much as a row as we can
		while ((i < grid_points) && (hc <= row_points)){
			mvwprintw(hex.win, hr, hc, "%c", hex.gc[i]);
			hc++;
			i++;
		}
		hc = 1;
		hr++;
	}
	wnoutrefresh(hex.win);
}

void v_refresh_ascii()
{
	box(ascii.win, 0, 0);
	
	int hr=1; // offset print row on grid. 1 avoids the borders
	int hc=1;
	int i=0; 
	while(i<hex.grid){
		// print as much as a row as we can
		while ((i < hex.grid) && (hc <= hex.digits)){
			mvwprintw(ascii.win, hr, hc, "%c", ascii.gc[i]);
			hc++;
			i++;
		}
		hc = 1;
		hr++;
	}
	wnoutrefresh(ascii.win);
}

void v_update_all_windows()
{
	v_populate_grids();

	refresh_status();
	refresh_helper("Options: quit insert edit delete test goto");
	v_refresh_hex();
	v_refresh_ascii();

	doupdate();
}

void size_windows()
{
	// Get the current screen dimensions
	getmaxyx(stdscr, app.rows, app.cols);
	app.too_small = false;
	
	// Calculate window heights 
	status.height = 4; // 2 lines of text + 2 for the box
	helper.height = 5; // 3 lines of text + 2 for the box
	hex.height = app.rows - status.height - helper.height;
	ascii.height = hex.height;
	
	hex.width = ((int)((app.cols-4) / 4) * 3.0) + 2;
	ascii.width = (int)((hex.width-2) / 3) + 2;
	status.width = hex.width + ascii.width;
	helper.width = status.width;

	hex.digits = ascii.width - 2;
	hex.rows = hex.height -2;
	hex.grid = hex.digits * hex.rows;
	
	// Check for minimum screen size to prevent crashes
	if (app.rows < (status.height + helper.height + 4) || app.cols < 68) {
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
		// Create new windows based on the new dimensions
		status.win = newwin(status.height, status.width, 0, 0);
		helper.win = newwin(helper.height, helper.width, app.rows - helper.height, 0);
		hex.win = newwin(hex.height, hex.width, status.height, 0);
		ascii.win = newwin(ascii.height, ascii.width, status.height, hex.width);
		
		// Draw borders and content for each window
		box(status.win, 0, 0);
		box(helper.win, 0, 0);
		box(hex.win, 0,0);
		box(ascii.win, 0, 0);
	}

	// simulate a move so that the changes in the grid size are reflected 
	v_handle_keys(KEY_ESCAPE);
	
	// populate the windows
	v_update_all_windows();
}

// Function to delete all windows
void delete_windows() 
{
	if (status.win != NULL) {
		delwin(status.win);
		status.win = NULL;
	}
	if (helper.win != NULL) {
		delwin(helper.win);
		helper.win = NULL;
	}
	if (hex.win != NULL) {
		delwin(hex.win);
		hex.win = NULL;
	}
	if (ascii.win != NULL) {
		delwin(ascii.win);
		ascii.win = NULL;
	}
}

void v_goto_byte() 
{
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	char *msg;
	char *endptr;
	char input_str[256] = "";
	unsigned long input_val;
 	msg=malloc(60);
 	
	sprintf(msg, "Goto range between 0 & %lu", (unsigned long)app.fsize);
	
	int msg_len = strlen(msg);
	int win_h = 4;                   // fixed height
	int win_w = msg_len + 4;         // width based on message
	if (win_w < 20) win_w = 20;      // minimum width
	
	int starty = (rows - win_h) / 2;
	int startx = (cols - win_w) / 2;
	
	// Create window and panel
	WINDOW *popup = newwin(win_h, win_w, starty, startx);
	PANEL  *panel = new_panel(popup);
	keypad(popup, TRUE); // Enable keyboard input for the window
	
	// Draw border and message
	box(popup, 0, 0);
	mvwprintw(popup, 1, 1, "%s", msg);
	mvwprintw(popup, 2, 1, "Byte:");
	
	// Show it
	wrefresh(popup);
	update_panels();
	doupdate();

	// Move the cursor to the input position and get input
	echo();
	mvwgetnstr(popup, 2, 7, input_str, sizeof(input_str) - 1);
	noecho();
	
	// Convert the string to an integer
	    // Convert string to unsigned long using strtoul
	errno = 0; // Clear errno before the call
	input_val = strtoul(input_str, &endptr, 10);
	
	// Check for conversion errors
	if ((errno == ERANGE && (input_val == ULONG_MAX || input_val == 0)) || 
			(endptr == input_str) || (*endptr != '\0')) {
	// Handle conversion error (e.g., number out of range, no digits found, or extra characters)
	input_val = 0;
	}

	// set this as the new view start
	hex.v_start = input_val;
	
	// Clean up
	free(msg);
	del_panel(panel);
	delwin(popup);
	
	update_panels();
	doupdate();
}





