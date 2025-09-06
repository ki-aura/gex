#include <ncurses.h>
#include <panel.h>
#include <string.h>

#define ROWS 3
#define COLS 10

static char grid[ROWS][COLS+1];
static int cursor_row = 0, cursor_col = 0;

// Function to redraw the grid
static void draw_grid(WINDOW *win) {
    werase(win);
    box(win, 0, 0);
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            mvwaddch(win, r+1, c+1, grid[r][c]);
        }
    }
    wmove(win, cursor_row+1, cursor_col+1);
    wrefresh(win);
}

// Normalize all terminal click sequences into a single logical click
static void handle_click(int row, int col, mmask_t bstate) {
    // You could handle multiple mouse events mapping to one logical click
    // For example:
    // BUTTON1_PRESSED + BUTTON1_RELEASED, BUTTON1_CLICKED, etc.
    // All treated the same: move cursor
    cursor_row = row;
    cursor_col = col;
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Initialize ncurses colors (optional)
    start_color();

    // Initialize grid with 'X'
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            grid[r][c] = 'X';
        }
        grid[r][COLS] = '\0';
    }

    // Create a window + panel for the grid
    WINDOW *win = newwin(ROWS+2, COLS+2, 1, 1);
    PANEL *pan = new_panel(win);

    // Enable mouse events: clicks, presses, releases, etc.
    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED |
              BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED, NULL);

    MEVENT ev;
    int ch;

    draw_grid(win);

    while ((ch = getch()) != 'q') {  // Quit on 'q'
        if (ch == KEY_MOUSE) {
            if (getmouse(&ev) == OK) {
                int r = ev.y - 2; // adjust for window border + position
                int c = ev.x - 2;
                if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
                    // Normalize multiple mouse sequences into a single click
                    if (ev.bstate & (BUTTON1_PRESSED | BUTTON1_RELEASED |
                                      BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED |
                                      BUTTON1_TRIPLE_CLICKED)) {
                        handle_click(r, c, ev.bstate);
                    }

                    // Debug info at the bottom
                    mvprintw(LINES-1, 0,
                             "Mouse event bstate=0x%x at row=%d col=%d    ",
                             ev.bstate, r, c);
                    clrtoeol();
                }
            } else {
                mvprintw(LINES-1, 0, "getmouse() failed");
                clrtoeol();
            }
        } else if (ch >= 'a' && ch <= 'z') {
            // Replace all grid letters with typed char
            for (int r = 0; r < ROWS; r++)
                for (int c = 0; c < COLS; c++)
                    grid[r][c] = (char)ch;

            // Clear debug line on keypress
            mvprintw(LINES-1, 0, "Keypress '%c'       ", ch);
            clrtoeol();
        }
        draw_grid(win);
        refresh();
    }

    endwin();
    return 0;
}
