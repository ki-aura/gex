#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <panel.h>
#include "khash.h"

#define LINE_LEN 30

// khash: map position -> char
KHASH_MAP_INIT_INT(changemap, char)

// For qsort
int cmp_int(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
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
    keypad(win, TRUE);
    box(win, 0, 0);

    char orig_line[LINE_LEN + 1] = "This is a sample line text....";
    for (int i = 0; i < LINE_LEN; i++)
        mvwaddch(win, 1, i + 1, orig_line[i]);
    wrefresh(win);

    khash_t(changemap) *changes = kh_init(changemap);
    int cur = 0;
    wmove(win, 1, cur + 1);
    wrefresh(win);

    int ch;
    while ((ch = wgetch(win)) != 27) { // ESC to exit
        switch (ch) {
            case KEY_LEFT:
                if (cur > 0) cur--;
                break;

            case KEY_RIGHT:
                if (cur < LINE_LEN - 1) cur++;
                break;

            case KEY_BACKSPACE:
            case 127:
                if (cur > 0) cur--;
                {
                    khiter_t k = kh_get(changemap, changes, cur);
                    if (k != kh_end(changes)) {
                        kh_del(changemap, changes, k);
                        mvwaddch(win, 1, cur + 1, orig_line[cur]);
                    }
                }
                break;

            default:
                if (ch >= 32 && ch <= 126) { // printable
                    char orig = orig_line[cur];
                    khiter_t k = kh_get(changemap, changes, cur);

                    if (ch == orig) {
                        if (k != kh_end(changes)) {
                            kh_del(changemap, changes, k);
                        }
                        mvwaddch(win, 1, cur + 1, orig);
                    } else {
                        int ret;
                        if (k == kh_end(changes)) {
                            k = kh_put(changemap, changes, cur, &ret);
                        }
                        kh_val(changes, k) = ch;

                        wattron(win, COLOR_PAIR(1));
                        mvwaddch(win, 1, cur + 1, ch);
                        wattroff(win, COLOR_PAIR(1));
                    }

                    if (cur < LINE_LEN - 1) cur++;
                }
                break;
        }
        wmove(win, 1, cur + 1);
        wrefresh(win);
    }

    // ---- COMMIT PHASE ----
    // Extract keys, sort them, apply changes
    int nkeys = kh_size(changes);
    int *keys = malloc(nkeys * sizeof(int));
    int idx = 0;
    for (khiter_t k = kh_begin(changes); k != kh_end(changes); ++k) {
        if (kh_exist(changes, k)) {
            keys[idx++] = kh_key(changes, k);
        }
    }

    qsort(keys, nkeys, sizeof(int), cmp_int);
    for (int i = 0; i < nkeys; i++) {
        khiter_t k = kh_get(changemap, changes, keys[i]);
        if (k != kh_end(changes)) {
            orig_line[keys[i]] = kh_val(changes, k);
        }
    }

    free(keys);
    kh_destroy(changemap, changes);

    // Shutdown ncurses
    del_panel(pan);
    delwin(win);
    endwin();

    // Print the final committed line
    printf("Final committed line:\n%s\n", orig_line);

    return 0;
}
