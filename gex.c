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

void initial_setup()
{
	// Initialize ncurses
	initscr();
	cbreak();		  // Line buffering disabled, Pass on everything
	noecho();		  // Don't echo input
	curs_set(0);		
	keypad(stdscr, true); 	 // Enable function keys (like KEY_RESIZE )
	set_escdelay(50);

	// general app defaults
	app.mode=VIEW_MODE;	

	// set up global variable for debug panel
	tmp = malloc(200);
	
	// Hex window
	hex.v_start = 0;
}

void final_close()
{
	// Clean up 
	delete_windows();
	clear();
	refresh();
	endwin();
	free(tmp);
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
			create_view_menu(status.win);
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
	mvwprintw(status.win, 1, 1, "Mode %s Fsize %i offset %i to %i       ", app_mode_desc[app.mode], 
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

bool popup_question(char *qline1, char *qline2, popup_types pt, app_mode mode) 
{
	bool answer;
	int ch;
	int qlen;
	// make sure we size to the longer of the question lines 
	qlen = (strlen(qline1) > strlen(qline2)) ? strlen(qline1) : strlen(qline2);
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
	curs_set(0);
	wrefresh(popup);
	update_panels();
	doupdate();
	
	switch(pt){
	case PTYPE_YN:
		ch = wgetch(popup);
		while ((ch != 'y') && (ch != 'n')) ch = wgetch(popup);
		answer = (ch == 'y');
		break;

	case PTYPE_CONTINUE:
		wgetch(popup);	
		answer = true;
		break;
	}

	// Clean up panel
	del_panel(panel);
	delwin(popup);	
	update_panels();

	// refresh edit mode
	if(mode == EDIT_MODE){
		curs_set(2);
		e_refresh_hex();
		e_refresh_ascii();
		doupdate();
	}

	// popup an are you sure dialog
	return answer;
}



int main(int argc, char *argv[]) 
{
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
				// cause issues so let global key handler deal with it
				handle_global_keys(ch);		
				// Re-create all windows on resize
				create_windows();
			} else {
				if (!app.too_small) handle_global_keys(ch);
			}
			ch = getch();
		}
		// no longer need the file open
		close_file();
	} 
	
	// tidy up
	final_close();		
	return 0;
}

// basic version 
void bcreate_view_menu(WINDOW *status_win)
{
    ITEM *items[6];
    MENU *menu;
    WINDOW *sub;
    int h, w;

    // Define items sequentially
    items[0] = new_item("View", NULL);
    items[1] = new_item("Edit", NULL);
    items[2] = new_item("Insert", NULL);
    items[3] = new_item("Delete", NULL);
    items[4] = new_item("Quit", NULL);
    items[5] = NULL;  // terminator

    // Create the menu
    menu = new_menu((ITEM **)items);

    // Get status window size
    getmaxyx(status_win, h, w);

    // Create a subwindow for the menu items, leave one line for the title and 1 char border
    sub = derwin(status_win, 1, w - 2, 2, 1);  // 1 row high, full width minus borders
    set_menu_win(menu, status_win);
    set_menu_sub(menu, sub);

    // Horizontal layout: 1 row, all items visible
    set_menu_format(menu, 1, 5);
    set_menu_mark(menu, "");   // no arrows

    // Enable arrow keys in status_win
    keypad(status_win, TRUE);

    // Draw border and title
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " View Menu ");

    // Post menu
    post_menu(menu);

    // Refresh both windows
    wrefresh(status_win);
    wrefresh(sub);

    int c;
    while ((c = wgetch(status_win)) != 'q') {
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
                break;
        }
        wrefresh(sub);        // update menu items
        wrefresh(status_win); // update border/title
    }

    // Cleanup
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; i < 5; i++)
        free_item(items[i]);
    delwin(sub);
}

// panel version
void pcreate_view_menu(WINDOW *status_win)
{
    ITEM *items[5];
    MENU *menu;
    PANEL *menu_panel;
    WINDOW *menu_win;
    WINDOW *sub;
    int h, w;

    // Define menu items
    items[0] = new_item("View", NULL);
    items[1] = new_item("Edit", NULL);
    items[2] = new_item("Insert", NULL);
    items[3] = new_item("Delete", NULL);
    items[4] = NULL;

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
    set_menu_format(menu, 1, 4);  // horizontal
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
    while ((c = wgetch(menu_win)) != 'q') {
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
    for (int i = 0; i < 4; i++)
        free_item(items[i]);

    hide_panel(menu_panel);
    del_panel(menu_panel);
    delwin(menu_win);
}

// dupwin()/copywin() version
void create_view_menu(WINDOW *status_win)
{
    ITEM *items[5];
    MENU *menu;
    WINDOW *menu_win, *menu_sub;
    WINDOW *backup;
    int h, w;

    // Define menu items
    items[0] = new_item("View", NULL);
    items[1] = new_item("Edit", NULL);
    items[2] = new_item("Insert", NULL);
    items[3] = new_item("Delete", NULL);
    items[4] = NULL;

    // Create the menu
    menu = new_menu((ITEM **)items);

    // Get status window size
    getmaxyx(status_win, h, w);

    // Backup the status window
    backup = dupwin(status_win);

    // Create a separate window for the menu
    int menu_h = 3;           // 1 row for items + border
    int menu_w = w - 4;
    int starty = 2;
    int startx = 2;
    menu_win = newwin(menu_h, menu_w, starty, startx);
    box(menu_win, 0, 0);

    // Subwindow inside menu_win for menu items
    menu_sub = derwin(menu_win, 1, menu_w - 2, 1, 1);
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, menu_sub);
    set_menu_format(menu, 1, 4); // horizontal
    set_menu_mark(menu, "");     // no arrows

    // Enable input on the menu window
    keypad(menu_win, TRUE);

    // Post the menu
    post_menu(menu);
    wrefresh(menu_win);
    wrefresh(menu_sub);

    // Event loop
    int c;
    while ((c = wgetch(menu_win)) != 'q') {
        switch (c) {
            case KEY_LEFT: menu_driver(menu, REQ_LEFT_ITEM); break;
            case KEY_RIGHT: menu_driver(menu, REQ_RIGHT_ITEM); break;
            case 10: // Enter
                mvwprintw(status_win, h - 2, 2,
                          "Selected: %s", item_name(current_item(menu)));
                wclrtoeol(status_win);
                wrefresh(status_win);
                break;
        }
        wrefresh(menu_sub);
        wrefresh(menu_win);
    }

    // Cleanup: remove menu and restore original content
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; i < 4; i++)
        free_item(items[i]);

    // Restore status window content
    copywin(backup, status_win, 0, 0, 0, 0, h - 1, w - 1, 0);
    wrefresh(status_win);

    delwin(menu_sub);
    delwin(menu_win);
    delwin(backup);
}




