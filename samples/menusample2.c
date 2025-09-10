#include <ncurses.h>
#include <panel.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    NONE,
    QUIT
} MenuResult;

void dummy1(void) {
    mvprintw(LINES - 2, 0, "Dummy1 called. Press any key...");
    getch();
}
void dummy2(void) {
    mvprintw(LINES - 2, 0, "Dummy2 called. Press any key...");
    getch();
}

MenuResult run_menu(void) {
    ITEM *items[4];
    MENU *menu;
    PANEL *menu_panel;
    WINDOW *menu_win;
    int c;
    MenuResult result = NONE;

    // Create items with names and aligned descriptions
    items[0] = new_item("Quit",   "q");
    items[1] = new_item("1dummy", "1");
    items[2] = new_item("2dummy", "2");
    items[3] = NULL;

    menu = new_menu(items);

    int win_height = 10;
    int win_width  = 40;
    int starty = (LINES - win_height) / 2;
    int startx = (COLS - win_width) / 2;

    menu_win = newwin(win_height, win_width, starty, startx);
    keypad(menu_win, TRUE);

    // Attach menu to window
// this woudl make it horizontal.    set_menu_format(menu, 1, 3);
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, derwin(menu_win, win_height - 4, win_width - 2, 3, 1));
    
    set_menu_mark(menu, " * ");

    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 1, "Use arrows, Enter, ESC");

    post_menu(menu);
    menu_panel = new_panel(menu_win);
    update_panels();
    doupdate();

    while ((c = wgetch(menu_win)) != 27) { // ESC
        switch (c) {
            case KEY_DOWN:
                menu_driver(menu, REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                menu_driver(menu, REQ_UP_ITEM);
                break;
            case 10: { // Enter
                ITEM *cur = current_item(menu);
                const char *name = item_name(cur);
                if (strcmp(name, "Quit") == 0) {
                    result = QUIT;
                    goto cleanup;
                } else if (strcmp(name, "1dummy") == 0) {
                    dummy1();
                } else if (strcmp(name, "2dummy") == 0) {
                    dummy2();
                }
                break;
            }
            case 'q': case 'Q':
                result = QUIT;
                goto cleanup;
            case '1':
                dummy1();
                break;
            case '2':
                dummy2();
                break;
        }
        update_panels();
        doupdate();
    }

cleanup:
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; items[i]; i++) free_item(items[i]);
    del_panel(menu_panel);
    delwin(menu_win);

    return result;
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int ch;
    MenuResult res = NONE;

    while (1) {
        mvprintw(LINES - 1, 0, "Press ESC to open menu, q to quit");
        refresh();
        ch = getch();
        if (ch == 27) { // ESC → show menu
            res = run_menu();
            if (res == QUIT) break;
        } else if (ch == 'q' || ch == 'Q') {
            break;
        }
    }

    endwin();
    return 0;
}
