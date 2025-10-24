#include "gex.h"


void size_windows(void) {
	// Get the current screen dimensions
	getmaxyx(stdscr, app.rows, app.cols);
	app.too_small = false;
	
	// Calculate window heights 
	status.height = 3; 
	hex.height = app.rows - status.height - 4; // 4 for the borders
	ascii.height = hex.height;					
	
	// calculate window widths
	hex.width = ((int)((app.cols-4) / 4) * 3); 
	ascii.width = (int)(hex.width / 3);
	status.width = hex.width + ascii.width + 2; // 2 for the internal borders of hex & ascii

	hex.grid = ascii.width * ascii.height;
	
	// Check for minimum screen size to prevent crashes
	// status.height (and hex) don't include borders, so +10 = 2 for status borders + 4+2 for hex 
	if (app.rows < (status.height + 8) || app.cols < 68) // 68 allows 16 bytes in each window
		// If the screen is too small, set the flag for window population
		app.too_small = true;
}

// Function to create and resize all windows
void create_windows(void) {
   // get sizes
   resizeterm(0, 0); 
   refresh();
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
			
		// Draw borders and content for each window
		box(status.border, 0, 0);
		box(hex.border, 0,0);
		box(ascii.border, 0, 0);
		
		status.win = newwin(status.height, status.width, 1, 1);
		hex.win = newwin(hex.height, hex.width, status.height+3, 1);
		ascii.win = newwin(ascii.height, ascii.width, status.height+3,(hex.width+3));
	}
	
	// simulate a move so that the changes in the grid size are reflected 
//	handle_keys(KEY_REFRESH
	
	// populate the windows
	update_all_windows();
}

// Function to delete all windows
void delete_windows(void) {
if (status.win != NULL) {delwin(status.win); status.win = NULL; }
if (hex.win != NULL) {delwin(hex.win); hex.win = NULL; }
if (ascii.win != NULL) {delwin(ascii.win); ascii.win = NULL; } 
if (status.border != NULL) {delwin(status.border); status.border = NULL; }
if (hex.border != NULL) {delwin(hex.border); hex.border = NULL; }
if (ascii.border != NULL) {delwin(ascii.border); ascii.border = NULL; } 
}

void refresh_status(void) {
	mvwprintw(status.win, 0, 0, "GEX %s [%s] Size:%lu Offset:%lu             ", 
			GEX_VERSION, get_filename(app.fname), app.fsize, cursor_full_file_offset());
	mvwprintw(status.win, 1, 0, "Grid offset %lu-%lu Screen:%dx%d Grid:%dx%d=%d           ", 
			hex.v_start, hex.v_start+hex.grid-1, app.rows, app.cols, ascii.width, hex.height, hex.grid);
	mvwprintw(status.win, 2, 0, 
			"cr%02d cc%02d cd%02d Hwin?%d hinib?%d lk%d lek%d chg%0d rbe?%d        ",
			hex.cur_row, hex.cur_col, hex.cur_digit, app.in_hex, hex.is_hinib, 
			app.lastkey, app.lasteditkey, RB_SIZE(), RB_EMPTY(&edits));
	box(status.border, 0, 0);
	wnoutrefresh(status.border);
	wnoutrefresh(status.win);
}

void refresh_grids(void){
	char hinib, lonib, a_char;
	int offset = 0, r=0, hc=0, ac=0;
	bool chg;
	
	while(file_offset_to_rc(offset, &r, &hc, &ac)) {	//while v_start + offset is < end of file
														// this also gives us grid coordinates
		// break the byte into displayable nibbles for the hex window
		byte_to_nibs(app.map[hex.v_start + offset], &hinib, &lonib);
		// check if there's an edit for this char
		search.offset = (size_t)(hex.v_start + offset);
		found = RB_FIND(edit_tree, &edits, &search);
		if (found) {
			// get the changed byte
			chg=true;
			byte_to_nibs(found->byte, &hinib, &lonib);
			a_char = byte_to_ascii(found->byte);
		} else {
			chg=false;
			a_char = byte_to_ascii(app.map[hex.v_start + offset]);
		}
		
		// update the grids
		if (chg) wattron(hex.win, COLOR_PAIR(1) | A_BOLD);	
		mvwprintw(hex.win, r, hc, "%c", hinib);
		mvwprintw(hex.win, r, hc+1, "%c", lonib);
		if (chg) wattroff(hex.win,COLOR_PAIR(1) |  A_BOLD);

		if (chg) wattron(ascii.win, COLOR_PAIR(1) | A_BOLD);	
		mvwprintw(ascii.win, r, ac, "%c", a_char);
		if (chg) wattroff(ascii.win,COLOR_PAIR(1) |  A_BOLD);
		offset++;			
	}
	
	box(hex.border, 0, 0);
	wnoutrefresh(hex.border);
	wnoutrefresh(hex.win);

	box(ascii.border, 0, 0);
	wnoutrefresh(ascii.border);
	wnoutrefresh(ascii.win);
}

void update_all_windows(void) {
	if (!app.too_small){	
		// refresh content
		refresh_grids();
		update_cursor();  // this does a doupdate();
	}
}

void update_cursor(void){
	// update status and debugging content
	refresh_status();
	// move the cursor
	if (app.in_hex) {
		wmove(hex.win, hex.cur_row, hex.cur_col);
		wnoutrefresh(hex.win);
	} else {
		wmove(ascii.win, hex.cur_row, hex.cur_digit);
		wnoutrefresh(ascii.win);	
	}
	doupdate(); 
}

unsigned long  popup_question(const char *qline1, const char *qline2, popup_types pt) {
	int ch, qlen, oldcs1, oldcs2;
	char *endptr;
	unsigned long answer;

	// make sure we size to the longer of the question lines (and at least 21 so a 20byte long can be typed)
	qlen = (int)((strlen(qline1) > strlen(qline2)) ? strlen(qline1) : strlen(qline2));
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
		mvwgetnstr(popup, 2, 1, tmp, 16); // 20 is max length of a 64bit unsigned long
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

void example_dynamic_rbtree(void); // prototype for test menu item
bool create_main_menu(void){
    const char *items[] = {
        "QUIT             (q)",
        "SAVE_Changes     (s)",
        "ABANDON_Changes  (a)",
        "GOTO_Byte        (g)",
        "INSERT_Bytes     (i)",
        "DELETE_Bytes     (d)",
        "Test Debug       (t)"
    };
    int mi = sizeof(items)/sizeof(items[0]);
    int highlight = 0;
    int choice = -1;
    int c, oldcurs;

    PANEL *menu_panel;
    WINDOW *menu_win;

    int win_height = mi + 4; // box + 2 header lines
    int win_width  = (int)strlen(items[0]) + 4; // box + space either side
    int starty = (LINES - win_height) / 2;
    int startx = (COLS - win_width) / 2;

    menu_win = newwin(win_height, win_width, starty, startx);
    keypad(menu_win, TRUE);

    menu_panel = new_panel(menu_win);
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 1, "Use arrows, Enter, ESC");
    oldcurs = curs_set(0); // store prev cursor state and turn it off

    do {
        // Draw menu with highlight
        for (int i = 0; i < mi; i++) {
            if (i == highlight) wattron(menu_win, A_REVERSE);
            else wattroff(menu_win, A_REVERSE);
			mvwprintw(menu_win, 3 + i, 2, "%s", items[i]);
		}
        update_panels();
        doupdate();

		c = wgetch(menu_win);
        switch (c) {
            case KEY_DOWN:
                highlight = (highlight + 1) % mi; break;
            case KEY_UP:
                highlight = (highlight - 1 + mi) % mi; break;
            case KEY_ENTER: 
            case KEY_MAC_ENTER: 
                choice = highlight; break;
            case 'q': case 'Q':
                choice = 0; break;
            case 's': case 'S':
                choice = 1; break;
            case 'a': case 'A':
                choice = 2; break;
            case 'g': case 'G':
                choice = 3; break;
            case 'i': case 'I':
                choice = 4; break;
            case 'd': case 'D':
                choice = 5; break;
            case 't': case 'T':
                choice = 6; break;
        }
        if (choice != -1) break; // break from do loop if valid choice
                    
    } while (c != KEY_ESCAPE && c != KEY_RESIZE);

	//put curser back
	curs_set(oldcurs);

    // Perform action
    switch (choice) {
        case 0: return TRUE; // QUIT
        case 1: save_changes(); break;
        case 2: abandon_changes(); break;
        case 3: handle_scrolling_movement(KEY_MOVE); break;
        case 4: insert_bytes(); break;
        case 5: delete_bytes(); break;
        case 6: example_dynamic_rbtree(); break;
        default: break;
    }

    del_panel(menu_panel);
    delwin(menu_win);
    update_all_windows();
    handle_global_keys(KEY_RESIZE);
    return FALSE;
}

void example_dynamic_rbtree(void)
{
	// create a dynamic tree, push 2 nodes on to it, find and delete them.

	// create a new tree
	struct edit_tree fred;
	RB_INIT(&fred);

	struct FByte *new_node;		// this will be malloc'd for each new node
	struct FByte s, *f; 		// s for search criteria, *f pointer to found node
	char proof[3] = "xy"; 		// update xy to ab and prove it worked

	popup_question(proof, "", PTYPE_CONTINUE);

	//push 7,a
	new_node = xmalloc(sizeof(*new_node));
	new_node->offset = 7;
	new_node->byte = 'a';
	RB_INSERT(edit_tree,&fred,new_node);

	// push 4,b
	new_node = xmalloc(sizeof(*new_node));
	new_node->offset = 4;
	new_node->byte = 'b';
	RB_INSERT(edit_tree,&fred,new_node);

	// find them and delete them
	s.offset = 7;
	f = RB_FIND(edit_tree, &fred, &s);
	if (f) {
		proof[0] = f->byte;
		RB_REMOVE(edit_tree, &fred, f);
		free(f);
	}

	s.offset = 4;
	f = RB_FIND(edit_tree, &fred, &s);
	if (f) {
		proof[1] = f->byte;
		RB_REMOVE(edit_tree, &fred, f);
		free(f);
	}

	popup_question(proof, "", PTYPE_CONTINUE);

}

