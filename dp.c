#include "dp.h"

// Debug Panel
// usage:  sprintf(tmp, "msg %lu %d", app.fsize , hex.grid); DP(tmp); 

void DP(const char *msg) 
{
	if(DP_ON){
	    int rows, cols;
	    getmaxyx(stdscr, rows, cols);
	
	    int msg_len = strlen(msg);
	    int win_h = 5;                   // fixed height
	    int win_w = msg_len + 4;         // width based on message
	    if (win_w < 20) win_w = 20;      // minimum width
	
	    int starty = (rows - win_h) / 2;
	    int startx = (cols - win_w) / 2;
	
	    // Create window and panel
	    WINDOW *popup = newwin(win_h, win_w, starty, startx);
	    PANEL  *panel = new_panel(popup);
	
	    // Draw border and message
	    box(popup, 0, 0);
	    mvwprintw(popup, 2, (win_w - msg_len) / 2, "%s", msg);
	
	    // Show it
	    update_panels();
	    doupdate();
	
	    // Wait for space
	    wgetch(popup);
	
	    // Clean up
	    del_panel(panel);
	    delwin(popup);
	
	    update_panels();
	    doupdate();
	}
}

