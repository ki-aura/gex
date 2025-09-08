#include "gex.h"
#include "file_handling.h"
#include "view_mode.h"
#include "edit_mode.h"

// Global variables
status_windef status = {.win = NULL, .border = NULL};
helper_windef helper = {.win = NULL, .border = NULL};
hex_windef hex = {.win = NULL, .border = NULL};
ascii_windef ascii = {.win = NULL, .border = NULL};
appdef app;
khiter_t slot;
int khret; // return value from kh_put calls - says if already exists
MEVENT event;
char *tmp = NULL;

///////////////////////////////////////////////////
// startup and close down
///////////////////////////////////////////////////

void initial_setup()
{
	// Initialize ncurses & set defaults 
	initscr();
	mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED |
          BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED |
          BUTTON1_TRIPLE_CLICKED, NULL);
	start_color();
	use_default_colors(); 
	init_pair(1, COLOR_RED, -1);
	cbreak();		  // Line buffering disabled, Pass on everything
	noecho();		  // Don't echo input
	curs_set(2);		
	keypad(stdscr, true); 	 // Enable function keys (like KEY_RESIZE )
	set_escdelay(50);	 // speed up recognition of escape key - don't wait 1 sec for possible escape sequence

	// create edit map for edit changes and undos
	app.edmap = kh_init(charmap);

	// general app defaults
//	app.mode=VIEW_MODE;	

	// set up global variable for debug & popup panel
	tmp = malloc(256); strcpy(tmp, " ");
	helper.helpmsg = malloc(256); strcpy(helper.helpmsg, " ");
	
	// Hex window offset to start of file
	hex.v_start = 0;
	app.in_hex = true;	// start in hex screen
	hex.cur_row=0;
	hex.cur_col=0;
	hex.cur_digit=0;	// first hex digit (takes 3 spaces)
	hex.is_hinib = true;	// left nibble of that digit
	
	// show cursor
	curs_set(2);
	wmove(hex.win, hex.cur_row, hex.cur_col);
	wrefresh(hex.win);

}

void final_close(int signum)
{
	// Clean up ncurses
	delete_windows();
	clear();
	refresh();
	endwin();
	
	// free any globals
	free(tmp);
	free(helper.helpmsg);
	
	// close out the hash
	kh_clear(charmap, app.edmap);
	kh_destroy(charmap, app.edmap);
	
	// user message for forced close
	if (signum == SIGINT){
		fputs("Ended by Ctrl+C\n", stderr);
		exit(EXIT_FAILURE);}
	if (signum == SIGQUIT){
		fputs("Ended by Ctrl+\\\n", stderr);
		exit(EXIT_FAILURE);}
	if (signum == SIGTERM){
		fputs("Programme Killed\n", stderr);
		exit(EXIT_FAILURE);}
}

///////////////////////////////////////////////////
// main logic
///////////////////////////////////////////////////

void handle_global_keys(int k) {
//int idx;
//unsigned char nibble;
bool undo = false;

	switch(k){
	case KEY_MOUSE: // only handle in Edit mode
		if ((getmouse(&event) == OK)) {
		// Treat any of these as a "logical click"
			const mmask_t CLICKY = BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | 
						BUTTON1_TRIPLE_CLICKED | BUTTON1_PRESSED;
			if (event.bstate & CLICKY) {
				int row, col;
				clickwin win = get_window_click(&event, &row, &col); // relative coords, or 'n'
				e_handle_click(win, row, col);
			}
		}
		break;

	case KEY_RESIZE:
		create_windows();
		handle_global_keys(KEY_REFRESH); // force an update
		v_update_all_windows();
		doupdate();
		break;
	
	case KEY_TAB:
	//ensure on left nib on ascii pane return
		if(!hex.is_hinib){
			hex.cur_col--;
			hex.is_hinib=true;
		}
		// flip panes
		app.in_hex = !app.in_hex;
		break; 

	case KEY_NCURSES_BACKSPACE:
	case KEY_MAC_DELETE:
		// first move left 
//		e_handle_keys(KEY_LEFT);
		// now set undo flag for any changes on this char or nibble 
		undo = true;
		// and trigger the delete


	} // end switch

	refresh_status();
	refresh_helper();
	// move the cursor
	if (app.in_hex) {
		wnoutrefresh(ascii.win);	
		wmove(hex.win, hex.cur_row, hex.cur_col);
		wnoutrefresh(hex.win);
	} else {
		wnoutrefresh(hex.win);
		wmove(ascii.win, hex.cur_row, hex.cur_digit);
		wnoutrefresh(ascii.win);	
	}
	doupdate();
//snprintf(tmp,200,"in hex %i", app.in_hex); popup_question(tmp, "", PTYPE_CONTINUE);

}

void refresh_status()
{
	box(status.border, 0, 0);
	mvwprintw(status.win, 0, 0, "Fsize %lu offset %lu-%lu Screen: %dr %dc grid %dx%d=%d           ", 
			app.fsize, hex.v_start, hex.v_end, app.rows, app.cols, ascii.width, hex.height, hex.grid);
	mvwprintw(status.win, 1, 0, "Hex: cr %d cc %d cd %d Hwin %d hinib %d    ",
			hex.cur_row, hex.cur_col, hex.cur_digit, app.in_hex, hex.is_hinib);
	wnoutrefresh(status.border);
	wnoutrefresh(status.win);
}

void refresh_helper()
{
	box(helper.border, 0, 0);
	char *help_line = malloc(helper.width + 1 ); // +1 for null
	memset(help_line, ' ', helper.width);
	help_line[helper.width]='\0';
	mvwprintw(helper.win, 0, 0, "%s", help_line);   // blank it out
	mvwprintw(helper.win, 0, 0, "%s", helper.helpmsg);     // and fill it new
	wnoutrefresh(helper.border);
	wnoutrefresh(helper.win);
	free(help_line);
}


clickwin get_window_click(MEVENT *event, int *row, int *col) 
{
    int win_rs, win_cs, win_re, win_ce; // rs row start, rs row end, ...
	
	// get mouse row (y) and col (x)
    int mr = event->y;
    int mc = event->x;

    // Check hex window
    getbegyx(hex.win, win_rs, win_cs); // row and col start
    getmaxyx(hex.win, win_re, win_ce); // row and col end

    if (mr >= win_rs && mr < win_rs + win_re &&
        mc >= win_cs && mc < win_cs + win_ce) { 
        *row = mr - win_rs;			
        *col = mc - win_cs;
        return WIN_HEX;
    }

    // Check ascii window
    getbegyx(ascii.win, win_rs, win_cs); // row and col start
    getmaxyx(ascii.win, win_re, win_ce); // row and col end

    if (mr >= win_rs && mr < win_rs + win_re &&
        mc >= win_cs && mc < win_cs + win_ce) { 
        *row = mr - win_rs;			
        *col = mc - win_cs;
        return WIN_ASCII;
    }

    // Outside both windows
    *row = *col = -1;
    return WIN_OTHER;
}

int main(int argc, char *argv[]) 
{
signal(SIGINT, final_close);
signal(SIGQUIT, final_close);
signal(SIGTERM, final_close);

	// Initial app setup
	initial_setup();
	// open file
	if (open_file(argc, argv)) {
		// and go....
		init_view_mode();
		create_windows();

		int ch = KEY_REFRESH; // doesn't trigger anything
		// Main loop to handle input
		while (ch != KEY_ESCAPE) {
			// if reresh, handle keys before we wait for another char
			if(ch == KEY_REFRESH) handle_global_keys(ch);
			ch = getch();
			handle_global_keys(ch);
		}
	}
	close_file(); 
	// tidy up
	final_close(0);		
	return 0;
}

