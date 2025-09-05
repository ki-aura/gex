#include "gex.h"
#include "dp.h"

// menu functions
 
// basic version 
void bcreate_view_menu(WINDOW *status_win)
{
    ITEM *items[6];
    MENU *menu;
    WINDOW *sub;
    int h, w;

    // Define items sequentially
    items[0] = new_item("View", NULL);
    items[1] = new_item("Edit", NULL);
    items[2] = new_item("Insert", NULL);
    items[3] = new_item("Delete", NULL);
    items[4] = new_item("Quit", NULL);
    items[5] = NULL;  // terminator

    // Create the menu
    menu = new_menu((ITEM **)items);

    // Get status window size
    getmaxyx(status_win, h, w);

    // Create a subwindow for the menu items, leave one line for the title and 1 char border
    sub = derwin(status_win, 1, w - 2, 2, 1);  // 1 row high, full width minus borders
    set_menu_win(menu, status_win);
    set_menu_sub(menu, sub);

    // Horizontal layout: 1 row, all items visible
    set_menu_format(menu, 1, 5);
    set_menu_mark(menu, "");   // no arrows

    // Enable arrow keys in status_win
    keypad(status_win, TRUE);

    // Draw border and title
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " View Menu ");

    // Post menu
    post_menu(menu);

    // Refresh both windows
    wrefresh(status_win);
    wrefresh(sub);

    int c;
    while ((c = wgetch(status_win)) != 'q') {
        switch (c) {
            case KEY_LEFT:
                menu_driver(menu, REQ_LEFT_ITEM);
                break;
            case KEY_RIGHT:
                menu_driver(menu, REQ_RIGHT_ITEM);
                break;
            case 10: // Enter
                mvwprintw(status_win, h - 2, 2,
                          "Selected: %s", item_name(current_item(menu)));
                wclrtoeol(status_win);
                break;
        }
        wrefresh(sub);        // update menu items
        wrefresh(status_win); // update border/title
    }

    // Cleanup
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; i < 5; i++)
        free_item(items[i]);
    delwin(sub);
}

// panel version
void pcreate_view_menu(WINDOW *status_win)
{
    ITEM *items[5];
    MENU *menu;
    PANEL *menu_panel;
    WINDOW *menu_win;
    WINDOW *sub;
    int h, w;

    // Define menu items
    items[0] = new_item("View", NULL);
    items[1] = new_item("Edit", NULL);
    items[2] = new_item("Insert", NULL);
    items[3] = new_item("Delete", NULL);
    items[4] = NULL;

    // Create the menu
    menu = new_menu((ITEM **)items);

    // Size & position for popup menu window
    getmaxyx(status_win, h, w);
    int menu_h = 3;         // 1 row for items + border
    int menu_w = w - 4;     // slightly narrower than status_win
    int starty = 2;         // below top border/title
    int startx = 2;

    menu_win = newwin(menu_h, menu_w, starty, startx);
    box(menu_win, 0, 0);

    // Subwindow for items inside menu_win
    sub = derwin(menu_win, 1, menu_w - 2, 1, 1);
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, sub);
    set_menu_format(menu, 1, 4);  // horizontal
    set_menu_mark(menu, "");      // no arrows
    keypad(menu_win, TRUE);

    // Create panel so it appears above status_win
    menu_panel = new_panel(menu_win);
    top_panel(menu_panel);
    update_panels();
    doupdate();

    post_menu(menu);
    wrefresh(menu_win);

    // Event loop
    int c;
    while ((c = wgetch(menu_win)) != 'q') {
        switch (c) {
            case KEY_LEFT:
                menu_driver(menu, REQ_LEFT_ITEM);
                break;
            case KEY_RIGHT:
                menu_driver(menu, REQ_RIGHT_ITEM);
                break;
            case 10: // Enter
                mvwprintw(status_win, h - 2, 2,
                          "Selected: %s", item_name(current_item(menu)));
                wclrtoeol(status_win);
                wrefresh(status_win);
                break;
        }
        wrefresh(sub);
        update_panels();
        doupdate();
    }

    // Cleanup
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; i < 4; i++)
        free_item(items[i]);

    hide_panel(menu_panel);
    del_panel(menu_panel);
    delwin(menu_win);
}

// dupwin()/copywin() version
void create_view_menu(WINDOW *status_win)
{
    ITEM *items[5];
    MENU *menu;
    WINDOW *menu_win, *menu_sub;
    WINDOW *backup;
    int h, w;

    // Define menu items
    items[0] = new_item("View", NULL);
    items[1] = new_item("Edit", NULL);
    items[2] = new_item("Insert", NULL);
    items[3] = new_item("Delete", NULL);
    items[4] = NULL;

    // Create the menu
    menu = new_menu((ITEM **)items);

    // Get status window size
    getmaxyx(status_win, h, w);

    // Backup the status window
    backup = dupwin(status_win);

    // Create a separate window for the menu
    int menu_h = 3;           // 1 row for items + border
    int menu_w = w - 4;
    int starty = 2;
    int startx = 2;
    menu_win = newwin(menu_h, menu_w, starty, startx);
    box(menu_win, 0, 0);

    // Subwindow inside menu_win for menu items
    menu_sub = derwin(menu_win, 1, menu_w - 2, 1, 1);
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, menu_sub);
    set_menu_format(menu, 1, 4); // horizontal
    set_menu_mark(menu, "");     // no arrows

    // Enable input on the menu window
    keypad(menu_win, TRUE);

    // Post the menu
    post_menu(menu);
    wrefresh(menu_win);
    wrefresh(menu_sub);

    // Event loop
    int c;
    while ((c = wgetch(menu_win)) != 'q') {
        switch (c) {
            case KEY_LEFT: menu_driver(menu, REQ_LEFT_ITEM); break;
            case KEY_RIGHT: menu_driver(menu, REQ_RIGHT_ITEM); break;
            case 10: // Enter
                mvwprintw(status_win, h - 2, 2,
                          "Selected: %s", item_name(current_item(menu)));
                wclrtoeol(status_win);
                wrefresh(status_win);
                break;
        }
        wrefresh(menu_sub);
        wrefresh(menu_win);
    }

    // Cleanup: remove menu and restore original content
    unpost_menu(menu);
    free_menu(menu);
    for (int i = 0; i < 4; i++)
        free_item(items[i]);

    // Restore status window content
    copywin(backup, status_win, 0, 0, 0, 0, h - 1, w - 1, 0);
    wrefresh(status_win);

    delwin(menu_sub);
    delwin(menu_win);
    delwin(backup);
}

