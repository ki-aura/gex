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

	// create edit map for edit changes and undos
	app.edmap = kh_init(charmap);

	// general app defaults

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
	
	return open_file(argc,argv);
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
	
	// close file
	close_file(); 
	
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

void handle_global_keys(int k) {

	switch(k){
	case KEY_MOUSE: // only handle in Edit mode
		if ((getmouse(&event) == OK)) {
		// Treat any of these as a "logical click"
			const mmask_t CLICKY = BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | 
						BUTTON1_TRIPLE_CLICKED | BUTTON1_PRESSED;
			if (event.bstate & CLICKY) {
				int row, col;
				clickwin win = get_window_click(&event, &row, &col); // relative coords, or 'n'
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
	if(initial_setup(argc, argv)){
		// everything opened fine... crack on!
		create_windows();
	
		int ch = KEY_REFRESH; // doesn't trigger anything
		// Main loop to handle input
		while (ch != KEY_SEND) {
			// if reresh, handle keys before we wait for another char
			// used by multiple functions to force a screen refresh
			if(ch == KEY_REFRESH) handle_global_keys(ch);
			
			if(ch == KEY_ESCAPE) 
				if(create_main_menu()) break; // true if quit is selected
			
			ch = getch();
app.lastkey = ch;
			handle_global_keys(ch);
		}

	}
	// tidy up
	final_close(0);		
	return 0;
}

// menu functions
 

// panel version
bool create_main_menu()
{
    int mi = 6;
    ITEM *items[mi+1]; // + null
    MENU *menu;
//    WINDOW *sub;
    PANEL *menu_panel;
    WINDOW *menu_win;
    int c;


    // Define items sequentially
    items[0] = new_item("QUIT", "q");
    items[1] = new_item("SAVE_Changes", "s");
    items[2] = new_item("ABANDON_Changes", "a");
    items[3] = new_item("GOTO_Byte", "g");
    items[4] = new_item("INSERT_Bytes", "i");
    items[5] = new_item("DELETE_Bytes", "d");
    items[6] = NULL;  // terminator

   menu = new_menu(items);

    int win_height = 10;
    int win_width  = 40;
    int starty = (LINES - win_height) / 2;
    int startx = (COLS - win_width) / 2;

    menu_win = newwin(win_height, win_width, starty, startx);
    keypad(menu_win, TRUE);

    // Attach menu to window
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, derwin(menu_win, win_height - 4, win_width - 2, 3, 1));
    
    set_menu_mark(menu, " * ");

    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 1, "Use arrows, Enter, ESC");

    post_menu(menu);
    menu_panel = new_panel(menu_win);
    update_panels();
    doupdate();

    while ((c = wgetch(menu_win)) != 27) { // ESC
        switch (c) {
            case KEY_DOWN:
                menu_driver(menu, REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                menu_driver(menu, REQ_UP_ITEM);
                break;
            case KEY_ESCAPE:
            	goto cleanup;
            	break;
            case 10: { // Enter
                ITEM *cur = current_item(menu);
                const char *name = item_name(cur);
                if (strcmp(name, item_name(items[0])) == 0) return TRUE; //quit
                else if (strcmp(name, item_name(items[1])) == 0) goto save;
                else if (strcmp(name, item_name(items[2])) == 0) goto abandon;
                else if (strcmp(name, item_name(items[3])) == 0) goto gotobyte;
                else if (strcmp(name, item_name(items[4])) == 0) goto insert;
                else if (strcmp(name, item_name(items[5])) == 0) goto delete;
                break;
            }
            case 'q': case 'Q':
                return TRUE; //quit
            case 's':
              	goto save; break;
            case 'a':
              	goto abandon; break;
            case 'g':
              	goto gotobyte; break;
            case 'i':
              	goto insert; break;
            case 'd':
              	goto delete; break;
        }
        update_panels();
        doupdate();
    }

//nothing selected
goto cleanup;
  
save:
	save_changes();
	goto cleanup;

abandon:
    abandon_changes();
	goto cleanup;

gotobyte:
	handle_scrolling_movement(KEY_MOVE);
	goto cleanup;

insert:

	goto cleanup;

delete:

	goto cleanup;

cleanup:
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; items[i]; i++) free_item(items[i]);

    hide_panel(menu_panel);
    del_panel(menu_panel);
    delwin(menu_win);

    update_all_windows();
    handle_global_keys(KEY_RESIZE);
    return FALSE;
}

