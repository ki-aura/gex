#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <panel.h>
#include "uthash.h"

#define LINE_LEN 30

// Structure for storing changes
typedef struct {
    int pos;            // position in the line (0..LINE_LEN-1)
    char ch;            // new character
    UT_hash_handle hh;  // makes this struct hashable
} change_t;

// Comparison function for HASH_SORT
int cmp_pos(change_t* a, change_t* b) {
    return a->pos - b->pos;
}

int main() {
    initscr();
    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLUE, -1); // highlight for changed chars
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    WINDOW* win = newwin(3, LINE_LEN + 2, 1, 1); // 1-line text + box
    PANEL* pan = new_panel(win);
    keypad(win, TRUE); // enable arrow keys for this window
    box(win, 0, 0);

    char orig_line[LINE_LEN + 1] = "This is a sample line text....";
    for(int i=0;i<LINE_LEN;i++)
        mvwaddch(win, 1, i+1, orig_line[i]);
    wrefresh(win);

    change_t* changes = NULL; // uthash table
    int cur = 0;
    wmove(win, 1, cur + 1);
    wrefresh(win);

    int ch;
    while ((ch = wgetch(win)) != 27) { // ESC to exit
        switch(ch) {
            case KEY_LEFT:
                if(cur>0) cur--;
                break;
            case KEY_RIGHT:
                if(cur<LINE_LEN-1) cur++;
                break;
            case KEY_BACKSPACE:
            case 127:
                if(cur>0) cur--;
                {
                    change_t* item = NULL;
                    HASH_FIND_INT(changes, &cur, item);
                    if(item) {
                        HASH_DEL(changes, item);
                        free(item);
                        mvwaddch(win, 1, cur+1, orig_line[cur]);
                    }
                }
                break;
            default:
                if(ch >= 32 && ch <= 126) { // printable
                    char orig = orig_line[cur];
                    change_t* item = NULL;
                    HASH_FIND_INT(changes, &cur, item);

                    if(ch == orig) {
                        if(item) {
                            HASH_DEL(changes, item);
                            free(item);
                        }
                        mvwaddch(win, 1, cur+1, orig);
                    } else {
                        if(!item) {
                            item = malloc(sizeof(change_t));
                            item->pos = cur;
                            HASH_ADD_INT(changes, pos, item);
                        }
                        item->ch = ch;
                        wattron(win, COLOR_PAIR(1));
                        mvwaddch(win, 1, cur+1, ch);
                        wattroff(win, COLOR_PAIR(1));
                    }

                    if(cur < LINE_LEN - 1) cur++;
                }
                break;
        }
        wmove(win, 1, cur+1);
        wrefresh(win);
    }

    // ---- COMMIT PHASE ----
    // Apply changes to the original line in sorted order
    HASH_SORT(changes, cmp_pos);
    change_t* s;
    change_t* tmp;
    HASH_ITER(hh, changes, s, tmp) {
        orig_line[s->pos] = s->ch;
        HASH_DEL(changes, s);
        free(s);
    }

    // Shutdown ncurses
    del_panel(pan);
    delwin(win);
    endwin();

    // Print the final committed line
    printf("Final committed line:\n%s\n", orig_line);

    return 0;
}
