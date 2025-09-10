/* menusample.c
   ncurses + panel + menu with horizontal menu and canonical hotkeys
   ESC opens/closes menu, ←/→ navigation, Enter activates, q/1/2 hotkeys.
*/

#include <ncurses.h>
#include <panel.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

static char *choices[] = {
    "Quit(q) ",   // show accelerator in label
    "1dummy(1) ",
    "2dummy(2) ",
    NULL
};

void dummy1(void) { /* placeholder */ }
void dummy2(void) { /* placeholder */ }

typedef enum { NONE, QUIT } action_t;

action_t run_menu(WINDOW *menu_win, PANEL *menu_panel)
{
    int n_choices = 0;
    while (choices[n_choices]) ++n_choices;

    ITEM **items = calloc(n_choices + 1, sizeof(ITEM *));
    for (int i = 0; i < n_choices; ++i)
        items[i] = new_item(choices[i], "");

    MENU *menu = new_menu((ITEM **)items);
    set_menu_format(menu, 1, n_choices);   // horizontal layout
    set_menu_mark(menu, "");               // no selection mark

    // create subwindow for menu content
    WINDOW *sub = derwin(menu_win, 1, getmaxx(menu_win) - 2, 1, 1);
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, sub);

    box(menu_win, 0, 0);
    post_menu(menu);

    show_panel(menu_panel);
    update_panels();
    doupdate();

    keypad(menu_win, TRUE);

    int c;
    action_t action = NONE;
    while ((c = wgetch(menu_win)) != ERR) {
        if (c == 27) { // ESC → exit back to main
            break;
        }
        switch (c) {
            case KEY_LEFT:
                menu_driver(menu, REQ_LEFT_ITEM);
                break;
            case KEY_RIGHT:
                menu_driver(menu, REQ_RIGHT_ITEM);
                break;
            case 10: // Enter
            case KEY_ENTER: {
                ITEM *cur = current_item(menu);
                const char *name = item_name(cur);
                if (strncasecmp(name, "Quit", 4) == 0) {
                    action = QUIT;
                    goto cleanup;
                } else if (name[0] == '1') {
                    dummy1();
                } else if (name[0] == '2') {
                    dummy2();
                }
                break;
            }
            // Hotkeys (q,1,2) regardless of current selection
            case 'q': case 'Q':
                action = QUIT;
                goto cleanup;
            case '1':
                set_current_item(menu, items[1]);
                dummy1();
                break;
            case '2':
                set_current_item(menu, items[2]);
                dummy2();
                break;
        }
        update_panels();
        doupdate();
    }

cleanup:
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; i < n_choices; ++i)
        free_item(items[i]);
    free(items);

    hide_panel(menu_panel);
    delwin(sub);
    update_panels();
    doupdate();

    return action;
}

int main(void)
{
    WINDOW *menu_win;
    PANEL *menu_panel;
    action_t action = NONE;
    int ch;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    menu_win = newwin(3, 40, 1, 1);
    menu_panel = new_panel(menu_win);
    hide_panel(menu_panel);

    mvprintw(6, 2, "Press ESC to open menu. Inside menu: ←/→, Enter, or q/1/2.");
    refresh();

    while (true) {
        ch = getch();
        if (ch == 27) { // ESC toggles menu
            action = run_menu(menu_win, menu_panel);
            if (action == QUIT) break;
            // redraw main screen after returning from menu
            touchwin(stdscr);
            refresh();
        }
    }

    endwin();
    return 0;
}
