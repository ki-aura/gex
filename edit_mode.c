#include "gex.h"
#include "edit_mode.h"


void e_handle_keys(int k)
{
	switch(k){
	// simple cases for home and end
	case KEY_HOME:
		// move start of current row
		hex.cur_col=1;
		hex.cur_digit=1;	// first hex digit (takes 3 spaces)
		hex.is_lnib = true;	// left nibble of that digit
		;break;

	case KEY_END:
		// move end of current row
		hex.cur_col = (hex.digits*3) - 2; // l nibble of last digit
		hex.cur_digit = hex.digits;	// first hex digit (takes 3 spaces)
		hex.is_lnib = true;	// left nibble of that digit
		break;

	case KEY_LEFT:
		// 1 hex.digits = lnib rnib space
		// boundary is 1 to hex.width -1
		if(app.in_hex){
			if (!hex.is_lnib){
				// we can safely move to left nib. don't move ascii cursor
				hex.cur_col--;
				hex.is_lnib=true;
			} else {
				// we're on lef nib. move back 2 unless at start of row, 
				// otherwise wrap back to end of row
				if(hex.cur_digit > 1){
					hex.cur_col-=2;
					hex.cur_digit--;
					hex.is_lnib=false;
				} else {
					hex.cur_col=(hex.digits*3) - 1; //1 this time as go to right nibble
					hex.cur_digit=hex.digits;	// first hex digit (takes 3 spaces)
					hex.is_lnib = false;	// left nibble of that digit
				}
			}
		
		} else { // we're in ascii pane
			if(hex.cur_digit > 1){
				hex.cur_col-=3;
				hex.cur_digit--;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col=(hex.digits*3) - 2;
					hex.cur_digit=hex.digits;	// last hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}

		break;
		
	case KEY_RIGHT:
		// 1 hex.digits = lnib rnib space
		// boundary is 1 to hex.width -1
		if(app.in_hex){
			if (hex.is_lnib){
				// we can safely move to right nib. don't move ascii cursor
				hex.cur_col++;
				hex.is_lnib=false;
			} else {
				// we're on right nib. move 2 unless at end of row, 
				// otherwise wrap back to start of row
				if(hex.cur_digit < hex.digits){
					hex.cur_col+=2;
					hex.cur_digit++;
					hex.is_lnib=true;
				} else {
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
				}
			}		
		} else { // we're in ascii pane
			if(hex.cur_digit < hex.digits){
				hex.cur_col+=3;
				hex.cur_digit++;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}
		break;
		
	case KEY_UP:
		if(hex.cur_row > 1) 
			hex.cur_row--;
		else 
			hex.cur_row = hex.rows;
		break;
		
	case KEY_DOWN:
		if(hex.cur_row < hex.rows)
			hex.cur_row++;
		else
			hex.cur_row = 1;
		break;

	case KEY_NPAGE:
		hex.cur_row = hex.rows;
		break;

	case KEY_PPAGE:
		hex.cur_row = 1;
		break;
		
	// do stuff
	case KEY_MAC_ENTER:
		e_save_changes();
		break;
		
	case KEY_TAB:
		//ensure on left nib on ascii pane return
		if(!hex.is_lnib){
			hex.cur_col--;
			hex.is_lnib=true;
		}
		// flip panes
		app.in_hex = !app.in_hex;
		break; 

// FOR DEBUG ONLY	
case KEY_SPACE:
hex.changes_made = true;	
break;
			
	default: 
		// handle non-movement / special keys 
		break;
	}

	// boundary conditions
	
	// move the cursor
	if (app.in_hex) {
		wmove(hex.win, hex.cur_row, hex.cur_col);
		wrefresh(hex.win);
	} else {
		wmove(ascii.win, hex.cur_row, hex.cur_digit);
		wrefresh(ascii.win);	
	}
}

void init_edit_mode()
{
	// set the helper bar to something mode helpful
	refresh_status(); wrefresh(status.win);
	refresh_helper("Options: Escape Enter Tab"); wrefresh(helper.win);
	
	// grab a screen copy
	e_copy_screen();
	e_refresh_hex();
	e_refresh_ascii();
	doupdate();
	
	// set edit mode coming in defaults
	app.in_hex = true;	// start in hex screen
	hex.changes_made = false;
	// cursor starting location
	hex.cur_row=1;
	hex.cur_col=1;
	hex.cur_digit=1;	// first hex digit (takes 3 spaces)
	hex.is_lnib = true;	// left nibble of that digit
	
	// show cursor
	curs_set(2);
	wmove(hex.win, hex.cur_row, hex.cur_col);
	wrefresh(hex.win);
}

void end_edit_mode()
{
	if( popup_question("Are you sure you want to exit Edit mode?",
			   "All changes will be lost (y/n)", PTYPE_YN, EDIT_MODE) ){
		// clean up
		curs_set(0);
		free(hex.gc_copy);
		free(ascii.gc_copy);
		// pass control back to main loop by setting mode back to view
		app.mode = VIEW_MODE;
	}	
}

void e_copy_screen()
{
	hex.gc_copy = malloc((hex.grid * 3) + 1);
	ascii.gc_copy = malloc(hex.grid + 1);

	strcpy(hex.gc_copy, hex.gc);
	strcpy(ascii.gc_copy, ascii.gc);
	
	// debug only
/*	ascii.gc_copy[0] = 'E';
	ascii.gc_copy[1] = 'D';
	ascii.gc_copy[2] = '!';

	hex.gc_copy[0] = 'E';
	hex.gc_copy[1] = 'D';
	hex.gc_copy[2] = '!';
*/
}

void e_refresh_hex()
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
			mvwprintw(hex.win, hr, hc, "%c", hex.gc_copy[i]);
			hc++;
			i++;
		}
		hc = 1;
		hr++;
	}
	wnoutrefresh(hex.win);
}

void e_refresh_ascii()
{
	box(ascii.win, 0, 0);
	
	int hr=1; // offset print row on grid. 1 avoids the borders
	int hc=1;
	int i=0; 
	while(i<hex.grid){
		// print as much as a row as we can
		while ((i < hex.grid) && (hc <= hex.digits)){
			mvwprintw(ascii.win, hr, hc, "%c", ascii.gc_copy[i]);
			hc++;
			i++;
		}
		hc = 1;
		hr++;
	}
	wnoutrefresh(ascii.win);
}

void e_save_changes(){
	if (!hex.changes_made)
		popup_question("No changes made",
			"Press any key to continue", PTYPE_CONTINUE, EDIT_MODE);
	else if(popup_question("Are you sure you want to save changes?",
			"This action can not be undone (y/n)", PTYPE_YN, EDIT_MODE)){
		// save changes 
		
		hex.changes_made = FALSE;
	}
}


///////////////////////////////////////
//////// ADDS cursor and nibble handling
/////////////////////////////////////
/*
Proper high/low nibble tracking when moving between bytes.
Cursor wrapping across rows and columns.
Switching panes (hex ↔ ASCII) without losing nibble state.
Editable visible screen only, using a screen copy for safe commit/discard.
mprovements Over Previous Version
Nibble state is preserved across cursor moves
Switching between hex and ASCII does not lose which nibble you’re editing.
Cursor wraps naturally
Moving right at end of row advances to next row.
Moving left at start of row wraps to previous row.
ASCII edits move cursor forward automatically
Standard behavior for hex editors.
Screen copy ensures safe undo/discard
Nothing is written to the mmap until commit.
Ready for pane-switching
Tab key toggles between hex and ASCII editing.
*/

// Render both hex + ASCII panes

typedef struct {
    unsigned char *data;       // mmap pointer
    int file_size;          // file size
} FileMap;

void rrender_screen(WINDOW *win, unsigned char *buffer, int rows, int cols) {
    werase(win);
    for (int r = 0; r < rows; r++) {
        wmove(win, r, 0);
        for (int c = 0; c < cols; c++) {
            wprintw(win, "%02X ", buffer[r*cols + c]);
        }
        wprintw(win, " | ");
        for (int c = 0; c < cols; c++) {
            unsigned char ch = buffer[r*cols + c];
            wprintw(win, "%c", isprint(ch) ? ch : '.');
        }
    }
    wrefresh(win);
}

// Move cursor safely and wrap rows/columns
void move_cursor(int *row, int *col, bool forward) {
    if (forward) {
        if (*col < hex.width - 1) (*col)++;
        else { *col = 0; if (*row < hex.height - 1) (*row)++; }
    } else {
        if (*col > 0) (*col)--;
        else { *col = hex.width - 1; if (*row > 0) (*row)--; }
    }
}

// Hex + ASCII edit loop
void hhex_ascii_edit(WINDOW *win, FileMap *fmap, int offset) {
    unsigned char screen_copy[hex.height * hex.width];
    int screen_bytes = hex.height * hex.width;
    if (offset + screen_bytes > fmap->file_size)
        screen_bytes = fmap->file_size - offset;

    memcpy(screen_copy, fmap->data + offset, screen_bytes);

    int row = 0, col = 0;
    bool in_ascii = false;
    bool high_nibble = true;

    curs_set(1);
    keypad(win, TRUE);

    rrender_screen(win, screen_copy, hex.height, hex.width);
    wmove(win, row, in_ascii ? (hex.width*3 + 3 + col) : (col*3));

    int ch;
    while ((ch = wgetch(win)) != '\n') {
        int idx = row * hex.width + col;

        switch (ch) {
            case KEY_UP:    if (row > 0) row--; break;
            case KEY_DOWN:  if (row < hex.height - 1) row++; break;
            case KEY_LEFT:  move_cursor(&row, &col, false); break;
            case KEY_RIGHT: move_cursor(&row, &col, true); break;
            case '\t':      in_ascii = !in_ascii; break;
            case 27:  // Escape
                if (/* confirmation popup */ true) {
                    memcpy(screen_copy, fmap->data + offset, screen_bytes);
                    rrender_screen(win, screen_copy, hex.height, hex.width);
                    goto exit_edit;
                }
                break;
            default:
                if (in_ascii) {
                    if (isprint(ch)) {
                        screen_copy[idx] = (unsigned char)ch;
                        move_cursor(&row, &col, true);
                    }
                } else {
                    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
                        unsigned char nibble = (ch >= '0' && ch <= '9') ? (ch - '0') :
                                               (ch >= 'a') ? (ch - 'a' + 10) : (ch - 'A' + 10);

                        if (high_nibble) {
                            screen_copy[idx] = (screen_copy[idx] & 0x0F) | (nibble << 4);
                            high_nibble = false;
                        } else {
                            screen_copy[idx] = (screen_copy[idx] & 0xF0) | nibble;
                            high_nibble = true;
                            move_cursor(&row, &col, true);
                        }
                    }
                }
                break;
        }

        // Update display & move cursor
        rrender_screen(win, screen_copy, hex.height, hex.width);
        wmove(win, row, in_ascii ? (hex.width*3 + 3 + col) : (col*3));
    }

    // Commit changes
    if (/* commit confirmation */ true) {
        memcpy(fmap->data + offset, screen_copy, screen_bytes);
        msync(fmap->data + offset, screen_bytes, MS_SYNC);
    }

exit_edit:
    curs_set(0);
    wrefresh(win);
}



