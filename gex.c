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
				handle_click(win, row, col);
				update_cursor();
			}
		}
		break;

	case KEY_RESIZE:
		create_windows();
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
	app.lastkey = ch;
			// if reresh, handle keys before we wait for another char
			// used by multiple functions to force a screen refresh
			if(ch == KEY_REFRESH) handle_global_keys(ch);
			
			if(ch == KEY_ESCAPE) create_view_menu(status.win);
			
			ch = getch();
			handle_global_keys(ch);
		}

	}
	// tidy up
	final_close(0);		
	return 0;
}

// menu functions
 

// panel version
void create_view_menu(WINDOW *status_win)
{
    int mi = 7;
    ITEM *items[mi];
    MENU *menu;
    WINDOW *sub;
    PANEL *menu_panel;
    WINDOW *menu_win;
    int h, w;

    // Define items sequentially
    items[0] = new_item("Quit", NULL);
    items[1] = new_item("Save Changes", NULL);
    items[2] = new_item("Abandon Changes", NULL);
    items[3] = new_item("Goto Byte", NULL);
    items[4] = new_item("Insert Bytes", NULL);
    items[5] = new_item("Delete Bytes", NULL);
    items[6] = NULL;  // terminator


    // Create the menu
    menu = new_menu((ITEM **)items);

    // Size & position for popup menu window
    getmaxyx(status_win, h, w);
    int menu_h = 3;         // 1 row for items + border
    int menu_w = w - 4;     // slightly narrower than status_win
    int starty = 2;         // below top border/title
    int startx = 2;

    menu_win = newwin(menu_h, menu_w, starty, startx);
    box(menu_win, 0, 0);

    // Subwindow for items inside menu_win
    sub = derwin(menu_win, 1, menu_w - 2, 1, 1);
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, sub);
    set_menu_format(menu, 1, (mi-1));  // horizontal
    set_menu_mark(menu, "");      // no arrows
    keypad(menu_win, TRUE);

    // Create panel so it appears above status_win
    menu_panel = new_panel(menu_win);
    top_panel(menu_panel);
    update_panels();
    doupdate();

    post_menu(menu);
    wrefresh(menu_win);

    // Event loop
    int c;
    while ((c = wgetch(menu_win)) != KEY_ESCAPE) {
        switch (c) {
            case KEY_LEFT:
                menu_driver(menu, REQ_LEFT_ITEM);
                break;
            case KEY_RIGHT:
                menu_driver(menu, REQ_RIGHT_ITEM);
                break;
            case 10: // Enter
                mvwprintw(status_win, h - 2, 2,
                          "Selected: %s", item_name(current_item(menu)));
                wclrtoeol(status_win);
                wrefresh(status_win);
                break;
        }
        wrefresh(sub);
        update_panels();
        doupdate();
    }

    // Cleanup
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; i < mi; i++)
        free_item(items[i]);

    hide_panel(menu_panel);
    del_panel(menu_panel);
    delwin(menu_win);
    update_all_windows();
    handle_global_keys(KEY_REFRESH);
}

