#include "gex.h"
#include "edit_mode.h"


///////////////////////////////////////
//////// BASIC EDIT FROM CHAT
/////////////////////////////////////
/*
Key Points in This Template:
Screen copy
screen_copy stores the visible bytes for edit.
Only committed to the mmap on Enter + confirmation.
Cursor movement
KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT move a cursor inside the visible pane.
Cursor coordinates multiplied by 3 for hex pane spacing.
Hex input handling
Accepts 0-9, A-F and updates high/low nibble alternately.
Can be extended to support ASCII pane edits, syncing back to hex.
Escape handling
Pops up a confirmation (placeholder) and discards changes if confirmed.
Commit changes
memcpy back to mmap.
msync() ensures data is written to disk.
*/

#define SCREEN_ROWS 16
#define SCREEN_COLS 16  // hex bytes per row

typedef struct {
    unsigned char *data;       // mmap pointer
    size_t file_size;          // file size
} FileMap;

// Render hex + ASCII panes from buffer
void render_hex(WINDOW *win, unsigned char *buffer, size_t rows, size_t cols) {
    werase(win);
    for (size_t r = 0; r < rows; r++) {
        wmove(win, r, 0);
        for (size_t c = 0; c < cols; c++) {
            wprintw(win, "%02X ", buffer[r*cols + c]);
        }
    }
    wrefresh(win);
}

// Minimal edit loop
void hex_edit(WINDOW *win, FileMap *fmap, size_t offset) {
    int ch;
    size_t cursor_row = 0, cursor_col = 0;

    size_t screen_bytes = SCREEN_ROWS * SCREEN_COLS;
    if (offset + screen_bytes > fmap->file_size)
        screen_bytes = fmap->file_size - offset;

    // Copy visible screen
    unsigned char screen_copy[SCREEN_ROWS * SCREEN_COLS];
    memcpy(screen_copy, fmap->data + offset, screen_bytes);

    curs_set(1);
    keypad(win, TRUE);

    render_hex(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
    wmove(win, cursor_row, cursor_col*3);  // 3 chars per byte in hex pane

    while ((ch = wgetch(win)) != '\n') {
        switch (ch) {
            case KEY_UP:
                if (cursor_row > 0) cursor_row--;
                break;
            case KEY_DOWN:
                if (cursor_row < SCREEN_ROWS - 1) cursor_row++;
                break;
            case KEY_LEFT:
                if (cursor_col > 0) cursor_col--;
                break;
            case KEY_RIGHT:
                if (cursor_col < SCREEN_COLS - 1) cursor_col++;
                break;
            case 27:  // Escape key
                if (/* show confirmation popup */ true) {
                    memcpy(screen_copy, fmap->data + offset, screen_bytes);
                    render_hex(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
                    goto exit_edit;
                }
                break;
            default:
                // Handle hex input: only accept 0-9 A-F
                if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
                    size_t idx = cursor_row * SCREEN_COLS + cursor_col;
                    unsigned char nibble = (ch >= '0' && ch <= '9') ? (ch - '0') :
                                           (ch >= 'a') ? (ch - 'a' + 10) : (ch - 'A' + 10);

                    // Example: overwrite high nibble first, then low nibble
                    static bool high_nibble = true;
                    if (high_nibble) {
                        screen_copy[idx] = (screen_copy[idx] & 0x0F) | (nibble << 4);
                        high_nibble = false;
                    } else {
                        screen_copy[idx] = (screen_copy[idx] & 0xF0) | nibble;
                        high_nibble = true;
                        // Move cursor right after completing byte
                        if (cursor_col < SCREEN_COLS - 1) cursor_col++;
                        else if (cursor_row < SCREEN_ROWS - 1) { cursor_col = 0; cursor_row++; }
                    }

                    render_hex(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
                }
                break;
        }
        wmove(win, cursor_row, cursor_col*3);
    }

    // Commit changes
    if (/* show commit confirmation */ true) {
        memcpy(fmap->data + offset, screen_copy, screen_bytes);
        msync(fmap->data + offset, screen_bytes, MS_SYNC);
    }

exit_edit:
    curs_set(0);
    wrefresh(win);
}

///////////////////////////////////////
//////// ADDS ASCII EDIT
/////////////////////////////////////
/*
How it works
Hex and ASCII panes
Hex: 3-character spacing per byte ("XX ").
ASCII: printable characters, dots for non-printable.
Tab switches between panes.
Cursor logic
cursor_row / cursor_col always refer to the visible byte.
Cursor column is offset in ASCII pane by (SCREEN_COLS*3 + 3).
Hex input
Accepts 0-9, A-F, a-f and alternates high/low nibble.
ASCII input
Only printable characters allowed; updates the same screen_copy.
Hex pane is the source of truth; ASCII edits modify the underlying byte, which will be reflected if user switches back.
Commit / discard
Screen copy is only committed on Enter + confirmation.
Escape with confirmation discards changes.
*/

// Render both hex + ASCII panes from buffer
void render_screen(WINDOW *win, unsigned char *buffer, size_t rows, size_t cols) {
    werase(win);
    for (size_t r = 0; r < rows; r++) {
        wmove(win, r, 0);
        for (size_t c = 0; c < cols; c++) {
            wprintw(win, "%02X ", buffer[r*cols + c]);
        }

        wprintw(win, " | ");  // separator
        for (size_t c = 0; c < cols; c++) {
            unsigned char ch = buffer[r*cols + c];
            wprintw(win, "%c", isprint(ch) ? ch : '.');
        }
    }
    wrefresh(win);
}

// Minimal edit loop with hex + ASCII pane
void hex_ascii_edit(WINDOW *win, FileMap *fmap, size_t offset) {
    int ch;
    size_t cursor_row = 0, cursor_col = 0;
    bool in_ascii = false;   // false: editing hex, true: editing ASCII
    static bool high_nibble = true;
    size_t idx;

    size_t screen_bytes = SCREEN_ROWS * SCREEN_COLS;
    if (offset + screen_bytes > fmap->file_size)
        screen_bytes = fmap->file_size - offset;

    // Copy visible screen for edit
    unsigned char screen_copy[SCREEN_ROWS * SCREEN_COLS];
    memcpy(screen_copy, fmap->data + offset, screen_bytes);

    curs_set(1);
    keypad(win, TRUE);

    render_screen(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
    wmove(win, cursor_row, in_ascii ? (SCREEN_COLS*3 + 3 + cursor_col) : (cursor_col*3));

    while ((ch = wgetch(win)) != '\n') {
        switch (ch) {
            case KEY_UP:
                if (cursor_row > 0) cursor_row--;
                break;
            case KEY_DOWN:
                if (cursor_row < SCREEN_ROWS - 1) cursor_row++;
                break;
            case KEY_LEFT:
                if (cursor_col > 0) cursor_col--;
                break;
            case KEY_RIGHT:
                if (cursor_col < SCREEN_COLS - 1) cursor_col++;
                break;
            case '\t':  // Tab: switch panes
                in_ascii = !in_ascii;
                break;
            case 27:  // Escape
                if (/* show confirmation popup */ true) {
                    memcpy(screen_copy, fmap->data + offset, screen_bytes);
                    render_screen(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
                    goto exit_edit;
                }
                break;
            default:
                idx = cursor_row * SCREEN_COLS + cursor_col;
                if (in_ascii) {
                    // Only printable characters
                    if (isprint(ch)) {
                        screen_copy[idx] = (unsigned char)ch;
                        // No cursor auto-advance; optional
                        if (cursor_col < SCREEN_COLS - 1) cursor_col++;
                        else if (cursor_row < SCREEN_ROWS - 1) { cursor_col = 0; cursor_row++; }
                    }
                } else {
                    // Hex pane input: accept 0-9, A-F, a-f
                    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
                        unsigned char nibble = (ch >= '0' && ch <= '9') ? (ch - '0') :
                                               (ch >= 'a') ? (ch - 'a' + 10) : (ch - 'A' + 10);
                        if (high_nibble) {
                            screen_copy[idx] = (screen_copy[idx] & 0x0F) | (nibble << 4);
                            high_nibble = false;
                        } else {
                            screen_copy[idx] = (screen_copy[idx] & 0xF0) | nibble;
                            high_nibble = true;
                            if (cursor_col < SCREEN_COLS - 1) cursor_col++;
                            else if (cursor_row < SCREEN_ROWS - 1) { cursor_col = 0; cursor_row++; }
                        }
                    }
                }
                break;
        }
        // Update display
        render_screen(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
        wmove(win, cursor_row, in_ascii ? (SCREEN_COLS*3 + 3 + cursor_col) : (cursor_col*3));
    }

    // Commit changes
    if (/* show commit confirmation */ true) {
        memcpy(fmap->data + offset, screen_copy, screen_bytes);
        msync(fmap->data + offset, screen_bytes, MS_SYNC);
    }

exit_edit:
    curs_set(0);
    wrefresh(win);
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
void rrender_screen(WINDOW *win, unsigned char *buffer, size_t rows, size_t cols) {
    werase(win);
    for (size_t r = 0; r < rows; r++) {
        wmove(win, r, 0);
        for (size_t c = 0; c < cols; c++) {
            wprintw(win, "%02X ", buffer[r*cols + c]);
        }
        wprintw(win, " | ");
        for (size_t c = 0; c < cols; c++) {
            unsigned char ch = buffer[r*cols + c];
            wprintw(win, "%c", isprint(ch) ? ch : '.');
        }
    }
    wrefresh(win);
}

// Move cursor safely and wrap rows/columns
void move_cursor(size_t *row, size_t *col, bool forward) {
    if (forward) {
        if (*col < SCREEN_COLS - 1) (*col)++;
        else { *col = 0; if (*row < SCREEN_ROWS - 1) (*row)++; }
    } else {
        if (*col > 0) (*col)--;
        else { *col = SCREEN_COLS - 1; if (*row > 0) (*row)--; }
    }
}

// Hex + ASCII edit loop
void hhex_ascii_edit(WINDOW *win, FileMap *fmap, size_t offset) {
    unsigned char screen_copy[SCREEN_ROWS * SCREEN_COLS];
    size_t screen_bytes = SCREEN_ROWS * SCREEN_COLS;
    if (offset + screen_bytes > fmap->file_size)
        screen_bytes = fmap->file_size - offset;

    memcpy(screen_copy, fmap->data + offset, screen_bytes);

    size_t row = 0, col = 0;
    bool in_ascii = false;
    bool high_nibble = true;

    curs_set(1);
    keypad(win, TRUE);

    rrender_screen(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
    wmove(win, row, in_ascii ? (SCREEN_COLS*3 + 3 + col) : (col*3));

    int ch;
    while ((ch = wgetch(win)) != '\n') {
        size_t idx = row * SCREEN_COLS + col;

        switch (ch) {
            case KEY_UP:    if (row > 0) row--; break;
            case KEY_DOWN:  if (row < SCREEN_ROWS - 1) row++; break;
            case KEY_LEFT:  move_cursor(&row, &col, false); break;
            case KEY_RIGHT: move_cursor(&row, &col, true); break;
            case '\t':      in_ascii = !in_ascii; break;
            case 27:  // Escape
                if (/* confirmation popup */ true) {
                    memcpy(screen_copy, fmap->data + offset, screen_bytes);
                    rrender_screen(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
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
        rrender_screen(win, screen_copy, SCREEN_ROWS, SCREEN_COLS);
        wmove(win, row, in_ascii ? (SCREEN_COLS*3 + 3 + col) : (col*3));
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







