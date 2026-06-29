#include "gex.h"

/* ---------- small helpers ---------- */

static inline void clamp_byte_cursor(void)
{
    if (hex.cur_byte < 0)
        hex.cur_byte = ascii.width - 1;
    else if (hex.cur_byte >= ascii.width)
        hex.cur_byte = 0;

    if (hex.cur_nibble < 0) hex.cur_nibble = 1;
    if (hex.cur_nibble > 1) hex.cur_nibble = 0;

    hex.cur_digit = hex.cur_byte;
    hex.is_hinib  = (hex.cur_nibble == 0);
    hex.cur_col   = hex.cur_byte * 3 + (app.in_hex ? hex.cur_nibble : 0);
}

static inline void move_horiz(int delta)
{
    if (app.in_hex) {
        int n = (hex.cur_byte * 2) + hex.cur_nibble + delta;
        int max = ascii.width * 2;

        if (n < 0)
            n = max - 1;
        else if (n >= max)
            n = 0;

        hex.cur_byte   = n / 2;
        hex.cur_nibble = n % 2;
    } else {
        hex.cur_byte += delta;
        hex.cur_nibble = 0;
    }

    clamp_byte_cursor();
}

void k_left(void)  { move_horiz(-1); }
void k_right(void) { move_horiz(+1); }

/* ---------- mouse ---------- */

void handle_click(clickwin win, int row, int col)
{
    if (win == WIN_OTHER)
        return;

    hex.cur_row = row;

    if (win == WIN_HEX) {
        app.in_hex = true;

        if (col < 0) col = 0;

        int b = col / 3;
        if (b >= ascii.width) b = ascii.width - 1;

        hex.cur_byte   = b;
        hex.cur_nibble = (col % 3 == 1) ? 1 : 0;
    }
    else if (win == WIN_ASCII) {
        app.in_hex     = false;
        hex.cur_byte   = (col < ascii.width) ? col : ascii.width - 1;
        hex.cur_nibble = 0;
    }

    clamp_byte_cursor();
}

/* ---------- delete ---------- */

void handle_delete(void)
{
    int idx;

    if (!hex.is_hinib)
        k_left();

    idx = row_digit_to_offset(hex.cur_row, hex.cur_byte);
    search.offset = (size_t) (hex.v_start + idx);

    found = RB_FIND(edit_tree, &edits, &search);
    if (found)
        RB_REMOVE_FB(&edits, found);

    update_all_windows();
}

/* ---------- in-grid movement ---------- */

void handle_in_screen_movement(int k)
{
    switch (k) {

    case KEY_TAB:
        app.in_hex = !app.in_hex;
        hex.cur_nibble = 0;
        clamp_byte_cursor();
        break;

    case KEY_NCURSES_BACKSPACE:
    case KEY_MAC_DELETE:
    case KEY_OTHER_DELETE:
        k_left();
        handle_delete();
        break;

    case KEY_LEFT:
        k_left();
        break;

    case KEY_RIGHT:
        k_right();
        break;

    case KEY_HOME:
        hex.cur_row    = 0;
        hex.cur_byte   = 0;
        hex.cur_nibble = 0;
        clamp_byte_cursor();
        break;

    case KEY_END:
        hex.cur_row    = hex.height - 1;
        hex.cur_byte   = ascii.width - 1;
        hex.cur_nibble = 0;
        clamp_byte_cursor();
        break;
    }

    update_cursor();
}

/* ---------- scrolling ---------- */

void handle_scrolling_movement(int k)
{
    switch (k) {

    case KEY_UP:
        if (hex.cur_row > 0) {
            hex.cur_row--;
            update_cursor();
        } else {
            hex.v_start = (hex.v_start > ascii.width)
                            ? hex.v_start - ascii.width
                            : 0;
            update_all_windows();
        }
        break;

    case KEY_DOWN:
        if (hex.cur_row < (hex.height - 1)) {
            hex.cur_row++;
            update_cursor();
        } else {
            if ((size_t)hex.grid > app.fsize)
                hex.v_start = 0;
            else if ((hex.v_start + hex.grid + ascii.width) < app.fsize)
                hex.v_start += ascii.width;
            else
                hex.v_start = app.fsize - hex.grid;

            update_all_windows();
        }
        break;

    case KEY_NPAGE:
        if ((size_t) hex.grid > app.fsize)
            hex.v_start = 0;
        else if (hex.v_start + (2 * hex.grid) < app.fsize)
            hex.v_start += hex.grid;
        else
            hex.v_start = app.fsize - hex.grid;

        update_all_windows();
        break;

    case KEY_PPAGE:
        if ((size_t) hex.grid > app.fsize)
            hex.v_start = 0;
        else if (hex.v_start > (size_t) hex.grid)
            hex.v_start -= hex.grid;
        else
            hex.v_start = 0;

        update_all_windows();
        break;

    case KEY_MOVE:
        snprintf(tmp, 60, "Goto Byte? (0-%lu)",
                 (unsigned long) app.fsize - 1);

        hex.v_start = popup_question(tmp, "", PTYPE_UNSIGNED_LONG);

        if ((size_t) hex.grid >= app.fsize)
            hex.v_start = 0;
        else if ((hex.v_start + hex.grid) > app.fsize)
            hex.v_start = app.fsize - hex.grid;

        update_all_windows();
        break;
    }
}

/* ---------- edits ---------- */

void handle_edit_keys(int k)
{
    int idx;
    unsigned char full_file_byte, full_edit_byte;
    bool valid_edit = false;

    idx = row_digit_to_offset(hex.cur_row, hex.cur_byte);

    if (hex.v_start + idx >= app.fsize)
        return;

    if (!app.in_hex) {
        if (isprint(k)) {
            if (k == app.map[hex.v_start + idx]) {
                search.offset = (size_t)(hex.v_start + idx);
                found = RB_FIND(edit_tree, &edits, &search);
                if (found)
                    RB_REMOVE_FB(&edits, found);
            } else {
                RB_INSERT_FB(&edits,
                             (size_t)(hex.v_start + idx),
                             (unsigned char)k);
            }
            valid_edit = true;
        }
    } else {
        if (isxdigit(k)) {
            full_file_byte = app.map[hex.v_start + idx];

            search.offset = (size_t)(hex.v_start + idx);
            found = RB_FIND(edit_tree, &edits, &search);

            full_edit_byte = found ? found->byte : full_file_byte;

            if (hex.is_hinib)
                apply_hinib_to_byte(&full_edit_byte, k);
            else
                apply_lonib_to_byte(&full_edit_byte, k);

            if (full_edit_byte == full_file_byte) {
                if (found)
                    RB_REMOVE_FB(&edits, found);
            } else {
                found = RB_INSERT_FB(&edits,
                        (size_t)(hex.v_start + idx),
                        full_edit_byte);
                if (found != NULL)
                    found->byte = full_edit_byte;
            }

            valid_edit = true;
        }
    }

    if (valid_edit) {
        app.lasteditkey = k;
        update_all_windows();
        k_right();
    }
}
