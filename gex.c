#include "gex.h"
#include "file_handling.h"
#include "view_mode.h"
#include "edit_mode.h"


// Global variables
appdef app = {.mode=VIEW_MODE};
status_windef status = {.win = NULL};
helper_windef helper = {.win = NULL};
hex_windef hex = {.win = NULL, .gc = NULL};
ascii_windef ascii = {.win = NULL, .gc = NULL};
char app_mode_desc[5][10] = {"Edit  ", "Insert", "Delete", "View  ", "Keys  "};
char *tmp = NULL;
khiter_t slot;
int khret;
MEVENT event;


void initial_setup()
{
	// Initialize ncurses
	initscr();
//	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
//	mousemask(BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED, NULL);
mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED |
          BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED |
          BUTTON1_TRIPLE_CLICKED, NULL);
	start_color();
	use_default_colors(); 
	init_pair(1, COLOR_RED, -1);
	
	cbreak();		  // Line buffering disabled, Pass on everything
	noecho();		  // Don't echo input
	curs_set(0);		
	keypad(stdscr, true); 	 // Enable function keys (like KEY_RESIZE )
	set_escdelay(50);

	// create edit map for edit changes and undos
	app.edmap = kh_init(charmap);

	// general app defaults
	app.mode=VIEW_MODE;	


	// set up global variable for debug & popup panel
	tmp = malloc(256);
	
	// Hex window
	hex.v_start = 0;
}

void final_close(int signum)
{
	// Clean up 
	delete_windows();
	clear();
	refresh();
	endwin();
	free(tmp);
	kh_destroy(charmap, app.edmap);

	
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

void handle_global_keys(int k)
{
	// first check if we're Esc out of any non-view mode or being forced out
	// due to a screen resize
	if ((k==KEY_ESCAPE) || (k==KEY_RESIZE)) {
		// exit gracefully from non-view modes
		switch (app.mode){
	    	case EDIT_MODE:
	    		end_edit_mode(k);
			break;
		case INSERT_MODE:
			app.mode=VIEW_MODE;			
			break;
		case DELETE_MODE:
			// call end delete mode routine
			break;	
		case VIEW_MODE: 
			// nothing to do in edit mode
			break; 
		}
		// if we're still in view mode, then edit / delete etc have cancelled 
		// the Esc so we need to leave screen handling to them
		if(app.mode == VIEW_MODE) {
			if (k==KEY_RESIZE) create_windows();
			v_update_all_windows();
			doupdate();
		}
	}
	
	switch(app.mode){
	case VIEW_MODE:
		// check if we've initiated a new mode
		switch(k){
		// special keys to change mode
		case 'e': 
			app.mode=EDIT_MODE; 
			init_edit_mode();
			break;
		case 'i': 
			app.mode=INSERT_MODE; 
			// TEST!!
//			create_view_menu(status.win);
			break;
		case 'd': 
			app.mode=DELETE_MODE; 
			break;
		// g is part of view mode
		case 'g': 
			v_goto_byte(); 	// popup gets new hex.v_start
			v_handle_keys(k);	//force recalcs using move
			v_update_all_windows();
			doupdate();
			break;
		// otherwise for all other keys hand off for view movement keys
		default: 	
			v_handle_keys(k); 
			v_update_all_windows();
			doupdate();
			break;
		}	
		break;
		
    	case EDIT_MODE:
		e_handle_keys(k);
		break;

    	case INSERT_MODE:
	
		break;
    	case DELETE_MODE:
	
		break;
	}	
}

void refresh_status()
{
	box(status.win, 0, 0);
	mvwprintw(status.win, 1, 1, "Mode %s Fsize %lu offset %lu to %lu       ", app_mode_desc[app.mode], 
					app.fsize, hex.v_start, hex.v_end);
	mvwprintw(status.win, 2, 1, "Screen: %d rows, %d cols, grid %dx%d=%d", app.rows, app.cols, 
					hex.digits, hex.rows, hex.grid);
	wnoutrefresh(status.win);
}

void refresh_helper(char *helpmsg)
{
	box(helper.win, 0, 0);
	char *help_line = malloc(helper.width -1 ); // -2 to exclude borders, +1 for null
	memset(help_line, ' ', helper.width - 2);
	help_line[helper.width - 2]='\0';
	mvwprintw(helper.win, 1, 1, help_line);   // blank it out
	mvwprintw(helper.win, 1, 1, helpmsg);     // and fill it new
	wnoutrefresh(helper.win);
	free(help_line);
}

void byte_to_hex(unsigned char b, char *out) 
{
    const char hex_digits[] = "0123456789ABCDEF";
    out[0] = hex_digits[b >> 4];    // high nibble
    out[1] = hex_digits[b & 0x0F];  // low nibble
    // out[2] is NOT null-terminated — just 2 chars
}

char byte_to_ascii(unsigned char b) 
{
    return (isprint(b) ? (char)b : '.');
}

int hex_char_to_value(char c) 
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;  // invalid hex char
}

unsigned char hex_to_byte(char high, char low) 
{
    int hi = hex_char_to_value(high);
    int lo = hex_char_to_value(low);
    if (hi < 0 || lo < 0) return 0; // or handle error
    return (hi << 4) | lo;
}

void DP(const char *msg)
{	// debug popup
	popup_question(msg, "", PTYPE_CONTINUE);
}

unsigned long  popup_question(const char *qline1, const char *qline2, popup_types pt) 
{
	int ch, qlen, oldcs1, oldcs2;
	char *endptr;
	unsigned long answer;

	// make sure we size to the longer of the question lines (and at least 21)
	qlen = (strlen(qline1) > strlen(qline2)) ? strlen(qline1) : strlen(qline2);
	qlen = (qlen < 21) ? 21 : qlen;
	// Create window and panel
	WINDOW *popup = newwin(4, (qlen+2), ((app.rows - 4) / 2), 
					((app.cols - (qlen+2)) / 2));
	PANEL  *panel = new_panel(popup);
	keypad(popup, TRUE); // Enable keyboard input for the window
	
	// Draw border and message
	box(popup, 0, 0);
	wattron(popup, A_BOLD);
	mvwprintw(popup, 1, 1, qline1);
	mvwprintw(popup, 2, 1, qline2);
	wattroff(popup, A_BOLD);
	
	// Show it
	oldcs1 = curs_set(0);
	update_panels();
	doupdate();
	
	switch(pt){
	case PTYPE_YN:	// don't end until y or n typed
		do {
			ch = wgetch(popup);
		} while ((ch != 'y') && (ch != 'n'));
		answer = (unsigned long)(ch == 'y');
		break;

	case PTYPE_CONTINUE: 	// end after any key
		wgetch(popup);	
		answer = (unsigned long)true;
		break;
	
	case PTYPE_UNSIGNED_LONG:	// get a new file location (or default to 0 if invalid input)
		// Move the cursor to the input position and get input
		echo(); oldcs2 = curs_set(2);
		mvwgetnstr(popup, 2, 1, tmp, 20); // 20 is max length of a 64bit unsigned long
		noecho(); curs_set(oldcs2);
		
		// Convert string to unsigned long using strtoul
		errno = 0; // Clear errno before the call
		answer = strtoul(tmp, &endptr, 10);
		
		// Check for conversion errors
		if (tmp[0] == '-' || endptr == tmp || *endptr != '\0' || errno == ERANGE)
			answer = 0;
		break;
	}

	// Clean up panel
	curs_set(oldcs1);
	hide_panel(panel);
	update_panels();
	doupdate();
	del_panel(panel);
	delwin(popup);

	return answer;
}

clickwin get_window_click(MEVENT *event, int *row, int *col) 
{
    int win_y, win_x, win_rows, win_cols;
	
	// get mouse row (y) and col (x)
    int y = event->y;
    int x = event->x;

    // Check hex window
    getbegyx(hex.win, win_y, win_x);
    getmaxyx(hex.win, win_rows, win_cols);

    if (y >= win_y + 1 && y < win_y + win_rows - 1 &&
        x >= win_x + 1 && x < win_x + win_cols - 1) { 
        *row = y - (win_y);			
        *col = x - (win_x);
        return WIN_HEX;
    }

    // Check ascii window
    getbegyx(ascii.win, win_y, win_x);
    getmaxyx(ascii.win, win_rows, win_cols);

    if (y >= win_y + 1 && y < win_y + win_rows - 1 &&
        x >= win_x + 1 && x < win_x + win_cols - 1) {
        *row = y - (win_y);
        *col = x - (win_x);
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
int handled = 0;

	// Initial app setup
	initial_setup();

	// open file
	if (open_file(argc, argv)) {
		// and go....
		create_windows();
		int ch;
		// Main loop to handle input
		ch = '6'; // meaningless key
		while (	(app.mode != VIEW_MODE) ||
			((app.mode == VIEW_MODE) && (ch != 'q'))) {

			ch = getch();
			switch(ch){
			case KEY_MOUSE: // only handle in Edit mode
			if ((getmouse(&event) == OK) && (app.mode == EDIT_MODE)) {
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
			// if not in view mode when the window is resized, then this could
			// cause issues so let global key handler deal with it
			handle_global_keys(ch);		
			// Re-create all windows on resize
			create_windows();
	//		ch = getch();
			break;
			
			default:
			if (!app.too_small) handle_global_keys(ch);
//			ch = getch();
			break;			
			}
		}
		// no longer need the file open
		close_file();
	} 
	
	// tidy up
	final_close(0);		
	return 0;
}



