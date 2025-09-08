#include "gex.h"
#include "file_handling.h"
#include "view_mode.h"
#include "edit_mode.h"


// Global variables
appdef app = {.mode=VIEW_MODE};
status_windef status = {.win = NULL, .border = NULL};
helper_windef helper = {.win = NULL, .border = NULL};
hex_windef hex = {.win = NULL, .border = NULL};
ascii_windef ascii = {.win = NULL, .border = NULL};
char app_mode_desc[5][10] = {"Edit  ", "Insert", "Delete", "View  ", "Keys  "};
char *tmp = NULL;
khiter_t slot;
int khret; // return value from kh_put calls - says if already exists
MEVENT event;

///////////////////////////////////////////////////
// New helper functions
///////////////////////////////////////////////////

// set byte to 0x41 char 'A'   byte= nib_to_hex('4', '1');
// helper first, main func follows
inline unsigned char nib_to_hexval(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	return 0;
}
inline unsigned char nibs_to_hex(char hi, char lo) {
	return (nib_to_hexval(hi) << 4) | nib_to_hexval(lo);
}

// deconstruct byte to it's hi and low nibbles: hex_to_nibs(byte, &hi, &lo);
inline void hex_to_nibs(unsigned char byte, char *hi, char *lo) {
    *hi = (byte >> 4) & 0xF;
    *lo = byte & 0xF;
}

///////////////////////////////////////////////////
// Old helper functions
///////////////////////////////////////////////////

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

unsigned long  popup_question(const char *qline1, const char *qline2, popup_types pt) 
{
	int ch, qlen, oldcs1, oldcs2;
	char *endptr;
	unsigned long answer;

	// make sure we size to the longer of the question lines (and at least 21 so a 20byte long can be typed)
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
	mvwprintw(popup, 1, 1, "%s", qline1);
	mvwprintw(popup, 2, 1, "%s", qline2);
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
	app.mode=VIEW_MODE;	

	// set up global variable for debug & popup panel
	tmp = malloc(256);
	
	// Hex window offset to start of file
	hex.v_start = 0;
	app.in_hex = true;	// start in hex screen
	hex.cur_row=0;
	hex.cur_col=0;
	hex.cur_digit=0;	// first hex digit (takes 3 spaces)
	hex.is_lnib = true;	// left nibble of that digit
	
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

void handle_global_keys(int k)
{
	// first check if we're Esc out of any non-view mode or being forced out
	// due to a screen resize
	if (k==KEY_RESIZE)
		create_windows();
		
	v_handle_keys(k); 
	v_update_all_windows();
	doupdate();
	
/*		switch(k){
		// special keys to change mode
		case KEY_F2: 
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
		}	*/
}

void refresh_status()
{
	box(status.border, 0, 0);
	mvwprintw(status.win, 0, 0, "Mode %s Fsize %lu offset %lu to %lu       ", app_mode_desc[app.mode], 
					app.fsize, hex.v_start, hex.v_end);
	mvwprintw(status.win, 1, 0, "Screen: %d rows, %d cols, grid %dx%d=%d", app.rows, app.cols, 
					ascii.width, hex.height, hex.grid);
	wnoutrefresh(status.border);
	wnoutrefresh(status.win);
}

void refresh_helper(char *helpmsg)
{
	box(helper.border, 0, 0);
	char *help_line = malloc(helper.width + 1 ); // +1 for null
	memset(help_line, ' ', helper.width);
	help_line[helper.width]='\0';
	mvwprintw(helper.win, 0, 0, "%s", help_line);   // blank it out
	mvwprintw(helper.win, 0, 0, "%s", helpmsg);     // and fill it new
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
		int ch = KEY_HELP; // doesn't trigger anything
		// Main loop to handle input
		while (	(app.mode != VIEW_MODE) ||
			((app.mode == VIEW_MODE) && (ch != KEY_ESCAPE))) {

			ch = getch();
			switch(ch){
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
				e_handle_keys(ch); // force an update
				v_update_all_windows();
				doupdate();
			break;
			
			default:
				//if (!app.too_small) handle_global_keys(ch);
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

