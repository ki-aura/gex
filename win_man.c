#include "gex.h"


void size_windows() {
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
	if (app.rows < (status.height + helper.height + 10) || app.cols < 68) 
		// If the screen is too small, set the flag for window population
		app.too_small = true;
}

// Function to create and resize all windows
void create_windows() {
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
//	handle_keys(KEY_REFRESH
	
	// populate the windows
	update_all_windows();
}

// Function to delete all windows
void delete_windows() {
if (status.win != NULL) {delwin(status.win); status.win = NULL; }
if (helper.win != NULL) {delwin(helper.win); helper.win = NULL; }
if (hex.win != NULL) {delwin(hex.win); hex.win = NULL; }
if (ascii.win != NULL) {delwin(ascii.win); ascii.win = NULL; } 
if (status.border != NULL) {delwin(status.border); status.border = NULL; }
if (helper.border != NULL) {delwin(helper.border); helper.border = NULL; }
if (hex.border != NULL) {delwin(hex.border); hex.border = NULL; }
if (ascii.border != NULL) {delwin(ascii.border); ascii.border = NULL; } 
}

void refresh_status() {
	box(status.border, 0, 0);
	mvwprintw(status.win, 0, 0, "Fsize %lu offset %lu-%lu Screen: %dr %dc grid %dx%d=%d           ", 
			app.fsize, hex.v_start, hex.v_end, app.rows, app.cols, ascii.width, hex.height, hex.grid);
	wnoutrefresh(status.border);
	wnoutrefresh(status.win);
}

void refresh_helper() {
	box(helper.border, 0, 0);
	char *help_line = malloc(helper.width + 1 ); // +1 for null
	memset(help_line, ' ', helper.width);
	help_line[helper.width]='\0';
	mvwprintw(helper.win, 0, 0, 
			"Hex: cr %d cc %d cd %d Hwin %d hinib %d h %d aw %d hw %d lk %d mxr %d mPcl %d   ",
			hex.cur_row, hex.cur_col, hex.cur_digit, app.in_hex, hex.is_hinib, 
			hex.height, ascii.width, hex.width, app.lastkey, hex.max_row, hex.map_copy_len);
	mvwprintw(helper.win, 1, 0, "%s", help_line);   // blank it out
	mvwprintw(helper.win, 1, 0, "%s", helper.helpmsg);     // and fill it new
	wnoutrefresh(helper.border);
	wnoutrefresh(helper.win);
	free(help_line);
	wnoutrefresh(helper.border);
	wnoutrefresh(helper.win);
}


void refresh_hex() {

///////////// CHECK E_REFRESH FOR CHANGE HANDLING
	char hinib, lonib;
	int hr=0, hc=0, i=0; 
	
	int grid_points = hex.grid * 3;
	while(i<grid_points){
		// print as much as a row as we can
		while ((i < grid_points) && (hc < hex.width)){
			byte_to_nibs(app.map[hex.v_start + i], &hinib, &lonib);			
			mvwprintw(hex.win, hr, hc, "%c", hinib);
			mvwprintw(hex.win, hr, hc+1, "%c", lonib);
			hc+=3; // 2 nibbles and a space
			i++;	// next byte
		}
		hc = 0;
		hr++;
	}
	box(hex.border, 0, 0);
	wnoutrefresh(hex.border);
	wnoutrefresh(hex.win);
}

void refresh_ascii() {
///////////// CHECK E_REFRESH FOR CHANGE HANDLING
	int ar=0, ac=0, i=0; 
	
	while(i<hex.grid){
		// print as much as a row as we can
		while ((i < hex.grid) && (ac < ascii.width)){
			mvwprintw(ascii.win, ar, ac, "%c", byte_to_ascii(app.map[hex.v_start + i]));
			ac++;
			i++;
		}
		ac = 0;
		ar++;
	}
	box(ascii.border, 0, 0);
	wnoutrefresh(ascii.border);
	wnoutrefresh(ascii.win);
}

void update_all_windows() {
	if (!app.too_small){	
		// refresh content
		refresh_hex();
		refresh_ascii();
		update_cursor();  // this does a doupdate();
	}
}

void update_cursor(){
	// update status and debugging content
	refresh_helper();
	refresh_status();
	// move the cursor
	if (app.in_hex) {
//		wnoutrefresh(ascii.win);	
		wmove(hex.win, hex.cur_row, hex.cur_col);
		wnoutrefresh(hex.win);
	} else {
//		wnoutrefresh(hex.win);	
		wmove(ascii.win, hex.cur_row, hex.cur_digit);
		wnoutrefresh(ascii.win);	
	}
	doupdate(); 
}
