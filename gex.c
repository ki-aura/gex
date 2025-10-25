#include "gex.h"
#include "systree.h"
#include "rbtree.h"
#include "gex_helper_funcs.h"
#include "keyb_man.h"
#include "win_man.h"
#include "file_handling.h"


// Global variables
volatile sig_atomic_t sigint_received = 0;
status_windef status = {.win = NULL, .border = NULL};
hex_windef hex = {.win = NULL, .border = NULL};
ascii_windef ascii = {.win = NULL, .border = NULL};
appdef app;
MEVENT event;
char *tmp = NULL;

struct FByte search;
struct FByte *found, *nod;


///////////////////////////////////////////////////
// startup and close down
///////////////////////////////////////////////////

bool initial_setup(int argc, char *argv[])
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
	putp(tigetstr("smcup")); // use alternative buffer

	// general app defaults

	// set up global variable for debug & popup panel
	tmp = xmalloc(256); strcpy(tmp, " ");
	
	// Hex window offset to start of file
	hex.v_start = 0;
	app.in_hex = true;	// start in hex screen
	hex.cur_row=0;
	hex.cur_col=0;
	hex.cur_digit=0;	// first hex digit (takes 3 spaces)
	hex.is_hinib = true;	// left nibble of that digit
	app.lasteditkey = 0;
	
	// show cursor
	curs_set(2);
	
	return open_file(argc,argv);
}


int final_close(void){
	// Clean up ncurses
	delete_windows();
	clear();
	refresh();
	putp(tigetstr("rmcup"));
	endwin();
	
	// free any globals
	free(tmp);
	
	// close out the hash
	RB_CLEAR_TREE(&edits);
	
	// close file
	close_file(); 		

    switch (sigint_received) {
        case 1: fputs("Ended by Ctrl+C\n", stderr); return EXIT_FAILURE;
        case 2: fputs("Ended by Ctrl+\\\n", stderr); return EXIT_FAILURE;
        case 3: fputs("Program Killed\n", stderr); return EXIT_FAILURE;
        case 4: fputs("Unknown Cause of Exit\n", stderr); return EXIT_FAILURE;
        default: return EXIT_SUCCESS;
	}
}

void handle_global_keys(int k) {

	switch(k){
	case KEY_MOUSE: 
		if ((getmouse(&event) == OK)) {
		// Treat any of these as a "logical click"
			const mmask_t CLICKY = BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | 
						BUTTON1_TRIPLE_CLICKED | BUTTON1_PRESSED;
			if (event.bstate & CLICKY) {
				int row, col;
				clickwin win = get_window_click(&row, &col); // relative coords, or 'n'
				handle_click(win, row, col);
				update_cursor();
			}
		}
		break;

	case KEY_RESIZE:
		create_windows();
		handle_in_screen_movement(KEY_HOME); // reset cursor location
		update_all_windows();
		break;
	
	// in-screen key movement
	case KEY_NCURSES_BACKSPACE:
	case KEY_MAC_DELETE:
	case KEY_OTHER_DELETE:
	case KEY_LEFT:
	case KEY_RIGHT:
	case KEY_HOME:
	case KEY_END:
	case KEY_TAB:
		handle_in_screen_movement(k);
		break;
	
	// 
	case KEY_UP:
	case KEY_DOWN:
	case KEY_NPAGE:
	case KEY_PPAGE:
	//case MENU_GOTO:
		handle_scrolling_movement(k);
		break;
	
	//default checks for editing
	default: 
		handle_edit_keys(k);

	} // end switch

}


clickwin get_window_click(int *row, int *col) 
{
    int win_rs, win_cs, win_re, win_ce; // rs row start, rs row end, ...
	
	// get mouse row (y) and col (x)
    int mr = event.y;
    int mc = event.x;

    // Check hex window
    getbegyx(hex.win, win_rs, win_cs); // row and col start
    getmaxyx(hex.win, win_re, win_ce); // row and col end

    if (mr >= win_rs && mr < win_rs + win_re &&
        mc >= win_cs && mc < win_cs + win_ce) { 
        *row = mr - win_rs;			
        *col = (mc - win_cs);
        return WIN_HEX;
    }

    // Check ascii window
    getbegyx(ascii.win, win_rs, win_cs); // row and col start
    getmaxyx(ascii.win, win_re, win_ce); // row and col end

    if (mr >= win_rs && mr < win_rs + win_re &&
        mc >= win_cs && mc < win_cs + win_ce) { 
        *row = mr - win_rs;			
        *col = (mc - win_cs);
        return WIN_ASCII;
    }

    // Outside both windows
    *row = *col = -1;
    return WIN_OTHER;
}


void signal_handler(int signum) {
	if (sigint_received == 0) {
		if      (signum == SIGINT)  sigint_received = 1;
		else if (signum == SIGQUIT) sigint_received = 2;
		else if (signum == SIGTERM) sigint_received = 3;
		else                        sigint_received = 4; // unknown signal
	}
}

void setup_signals(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char *argv[]) 
{
	// handle signal interupts
	setup_signals();
	// Initial app setup
	if(initial_setup(argc, argv)){
		// everything opened fine... crack on!
		create_windows();

		int ch = KEY_REFRESH;
		
		// Main loop to handle input
		while (sigint_received == 0) {
			// Handle forced refresh before waiting for another key
			if (ch == KEY_REFRESH)
				handle_global_keys(ch);
		
			// Process ESC → main menu
			if (ch == KEY_ESCAPE) {
				int quit_selected = create_main_menu();
				if (quit_selected) {
					int unsaved = (RB_SIZE() != 0);
					if (!unsaved) {
						break; // nothing to lose
					}
		
					int confirm = popup_question(
						"Abandon unsaved changes?",
						"This action can not be undone (y/n)",
						PTYPE_YN
					);
		
					if (confirm)
						break;     // abandon changes
					else
						continue;
				}
			}
		
			// Normal key handling
			ch = getch();
			app.lastkey = ch;
			handle_global_keys(ch);
		}
	} else {
		// initial setup failed
		putp(tigetstr("rmcup"));
		endwin();
		fputs("File does not exist\n", stderr);
	}
	// tidy up
	int rc = final_close();		
	return rc;
}
