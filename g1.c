#include <ncurses.h>
#include <panel.h>
#include <string.h>

#define ROWS 3
#define COLS 10

static char grid[ROWS][COLS+1];
static int cursor_row = 0, cursor_col = 0;

// Selection state
static int sel_active = 0;
static int sel_start_row = -1, sel_start_col = -1;
static int sel_end_row = -1, sel_end_col = -1;

// Compare positions (row,col) lexicographically
static int before(int r1, int c1, int r2, int c2) {
    return (r1 < r2) || (r1 == r2 && c1 <= c2);
}

// Check if cell is in current selection
static int in_selection(int r, int c) {
    if (!sel_active) return 0;
    if (before(sel_start_row, sel_start_col, sel_end_row, sel_end_col)) {
        return before(sel_start_row, sel_start_col, r, c) &&
               before(r, c, sel_end_row, sel_end_col);
    } else {
        return before(sel_end_row, sel_end_col, r, c) &&
               before(r, c, sel_start_row, sel_start_col);
    }
}

// Redraw the grid
static void draw_grid(WINDOW *win) {
    werase(win);
    box(win, 0, 0);
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (in_selection(r, c)) {
                wattron(win, A_REVERSE);
                mvwaddch(win, r+1, c+1, grid[r][c]);
                wattroff(win, A_REVERSE);
            } else {
                mvwaddch(win, r+1, c+1, grid[r][c]);
            }
        }
    }
    wmove(win, cursor_row+1, cursor_col+1);
    wrefresh(win);
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | REPORT_MOUSE_POSITION, NULL);

    // Fill grid with X
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            grid[r][c] = 'X';
        }
        grid[r][COLS] = '\0';
    }

    WINDOW *win = newwin(ROWS+2, COLS+2, 1, 1);
    PANEL *pan = new_panel(win);

    MEVENT ev;
    int ch;
    int dragging = 0;

    draw_grid(win);

    while ((ch = getch()) != 'q') {
        if (ch == KEY_MOUSE) {
            if (getmouse(&ev) == OK) {
                int r = ev.y - 2; // adjust for window border + position
                int c = ev.x - 2;
                if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
                    if (ev.bstate & BUTTON1_PRESSED) {
                        dragging = 1;
                        sel_active = 1;
                        sel_start_row = r;
                        sel_start_col = c;
                        sel_end_row = r;
                        sel_end_col = c;
                        cursor_row = r;
                        cursor_col = c;
                    } else if ((ev.bstate & REPORT_MOUSE_POSITION) && dragging) {
                        sel_end_row = r;
                        sel_end_col = c;
                        cursor_row = r;
                        cursor_col = c;
                    } else if (ev.bstate & BUTTON1_RELEASED) {
                        dragging = 0;
                        sel_end_row = r;
                        sel_end_col = c;
                        cursor_row = r;
                        cursor_col = c;
                    }
                }
            }
        } else if (ch >= 'a' && ch <= 'z') {
            // Replace all grid letters with typed char
            for (int r = 0; r < ROWS; r++)
                for (int c = 0; c < COLS; c++)
                    grid[r][c] = (char)ch;
            sel_active = 0; // clear selection
        }
        draw_grid(win);
    }

    endwin();
    return 0;
}
