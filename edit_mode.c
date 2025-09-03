#include "gex.h"
#include "edit_mode.h"


void handle_edit_keys(int k)
{
//DP("in move mode");
DP_ON = true;

	switch(k){
	// simple cases for home and end
	case KEY_HOME:
		;break;
	case KEY_END:
		break;
	case KEY_LEFT:
		break;
	case KEY_RIGHT:
		break;
	case KEY_UP:
		break;
	case KEY_DOWN:
		break;
	case KEY_MAC_ENTER:
		break;
	case KEY_ESCAPE:
		break;
	case KEY_TAB:
		break;
	default: 
		break;
	}

	// boundary conditions
}

void init_edit_mode()
{
	// set the helper bar to something mode helpful
	refresh_status(); wrefresh(status.win);
	refresh_helper("Options: Escape Enter Tab"); wrefresh(helper.win);
	
	// put a cursor up
	curs_set(2);
	wmove(hex.win, 1, 1);
	wrefresh(hex.win);
}

void end_edit_mode()
{
	curs_set(0);

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



