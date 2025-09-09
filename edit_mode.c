#include "gex.h"


void e_init_view_mode()
{
	// grab a screen copy
//	e_copy_screen();
	e_refresh_hex();
	e_refresh_ascii();
	doupdate();
	
	// set edit mode coming in defaults
	app.in_hex = true;	// start in hex screen
	// cursor starting location
	hex.cur_row=0;
	hex.cur_col=0;
	hex.cur_digit=0;	// first hex digit (takes 3 spaces)
	hex.is_hinib = true;	// left nibble of that digit
	
	// show cursor
	curs_set(2);
	wmove(hex.win, hex.cur_row, hex.cur_col);
	wrefresh(hex.win);
}

void e_end_edit_mode(int k)
{
	bool e_exit = false; // default is not to 
	
	// if screen resized then no option, otherwise ask
	if(k==KEY_RESIZE) e_exit = true;
	else 
		if(kh_size(app.edmap) == 0) e_exit = true;
		else 
			if( popup_question("Are you sure you want to exit Edit mode?",
			"All unsaved changes will be lost (y/n)", PTYPE_YN) )
				e_exit = true;
		
	if (e_exit){
		// clean up
		curs_set(0);
		// pass control back to main loop by setting mode back to view
		update_all_windows();
	}	
}

void e_build_grids_from_map_copy()
{
	// we want to start with blank grids - every cell = space
//	memset(hex.gc_copy, ' ', (hex.grid * 3));
//	memset(ascii.gc_copy, ' ', (hex.grid));
/*	
	char t_hex[2];
	for(int i=0; i < (int)hex.map_copy_len; i++){
		// for the hex map copy, convert byte to ascii
		byte_to_hex(app.map[hex.v_start + i], t_hex);
		// we only need populat the hex digits as digit 3 is already a space
		//hex.gc_copy[i * 3] = t_hex[0];
		//hex.gc_copy[(i * 3) +1] = t_hex[1];		
		// for ascii it's a one to one conversion to ascii
		//ascii.gc_copy[i] = byte_to_ascii(app.map[hex.v_start + i];
	}
*/
}


void e_copy_screen()
{
	// copy the segment of file relating to the grid
	// from hex.v_start to either + size of grid in bytes or end of file whichever is smallest
	hex.map_copy_len = hex.v_end - hex.v_start +1;
	
//	int max_row;	// this is the max row we can edit if screen > file size
//	int max_col; 	// this is the max col on the max row we can edit if screen > file size
//	int max_digit; // this is the max digit on the max row we can edit if screen > file size
	hex.max_row = hex.map_copy_len / ascii.width; // number of full rows

	if((hex.max_row * ascii.width) == hex.map_copy_len) {
		// we ended up with exactly a full row
		hex.max_digit = ascii.width;
		hex.max_col = ascii.width * 3;
	} else {
		// we ended up with a partly filled row so can only copy up to there
		hex.max_digit = hex.map_copy_len - (hex.max_row * ascii.width);
		hex.max_col = hex.max_digit * 3;
		hex.max_row++; // we can go to next row, but only so far along
	}

/*	// allocate space and copy from the app.map
	hex.map_copy = malloc(hex.map_copy_len + 1);
	memcpy(hex.map_copy, (app.map + hex.v_start), hex.map_copy_len);
	hex.map_copy[hex.map_copy_len] = '\0';

	// now the displayable grid contents
	hex.gc_copy = malloc((hex.grid * 3) + 1);
	ascii.gc_copy = malloc(hex.grid + 1);

	e_build_grids_from_map_copy();
*/
}


void e_refresh_hex()
{
	box(hex.win, 0, 0);
	bool chg;
			char t_hex[2];

	int hr=0; // offset print row on grid. 1 avoids the borders
	int hc=0;
	int i=0; 
	int hex_pos=1; // tracks where we are in a hex digit
	int grid_points = hex.grid * 3;
	int row_points = ascii.width * 3;
	while(i<grid_points ){
		// print as much as a row as we can
		while ((i < grid_points) && (hc < row_points)){
			// check if there's a change at this point and bold it
			// we only want to check if the hex_pos is 1, and then it lasts for 3 postions

			if(hex_pos == 1) { // start of a hex digit, so we can safely / 3 for file offset
				slot = kh_get(charmap, app.edmap, (int64_t)(app.map + hex.v_start + (i/3)));
				if (slot != kh_end(app.edmap)) chg=true; else chg=false;
			}
			// output the hex with changes in red			
			if (chg) wattron(hex.win, COLOR_PAIR(1) | A_BOLD);
			
/*			byte_to_hex(app.map[hex.v_start + i], t_hex);			
			mvwprintw(hex.win, hr, hc, "%c", t_hex[0]);
			mvwprintw(hex.win, hr, hc+1, "%c", t_hex[1]);

*/
//			mvwprintw(hex.win, hr, hc, "%c", hex.map[i]);
			if (chg) wattroff(hex.win,COLOR_PAIR(1) |  A_BOLD);

			hc++;	// next col (3 cols to a digit)
			i++;
			hex_pos++; if(hex_pos > 3) hex_pos = 1; 
		}
		hc = 0;
		hr++;
	}
	wnoutrefresh(hex.win);
}

void e_refresh_ascii()
{
	box(ascii.border, 0, 0);
			char t_hex[2];
bool chg;
	int hr=0; // offset print row on grid. 1 avoids the borders
	int hc=0;
	int i=0; 
	while(i<hex.grid){
		// print as much as a row as we can
		while ((i < hex.grid) && (hc < ascii.width)){
			// check if there's a change at this point and bold it
			slot = kh_get(charmap, app.edmap, (int64_t)(app.map + hex.v_start + i));
			if (slot != kh_end(app.edmap)) chg=true; else chg=false;
			if (chg) wattron(ascii.win, COLOR_PAIR(1) | A_BOLD);			
//			mvwprintw(ascii.win, hr, hc, "%c", ascii.gc_copy[i]);
			if (chg) wattroff(ascii.win, COLOR_PAIR(1) | A_BOLD);
			hc++;
			i++;
		}
		hc = 0;
		hr++;
	}
	wnoutrefresh(ascii.border);
	wnoutrefresh(ascii.win);
}

void e_save_changes(){
	if (kh_size(app.edmap) == 0)
		popup_question("No changes made",
			"Press any key to continue", PTYPE_CONTINUE);
	else if(popup_question("Are you sure you want to save changes?",
			"This action can not be undone (y/n)", PTYPE_YN)){
		// save changes 
		// replace app.map segment with update changes & msync
	//	memcpy((app.map + hex.v_start), hex.map_copy, hex.map_copy_len);
		
		// and sync it out
		msync(app.map, app.fsize, MS_SYNC);
		// clear change history as these are now permanent
		kh_clear(charmap, app.edmap);
		// refresh to get rid of old change highlights
		e_build_grids_from_map_copy();
		e_refresh_ascii();
		e_refresh_hex();

		e_end_edit_mode(KEY_HELP); // doesn't trigger anything
		e_handle_keys(KEY_HELP); // just triggers the screen refresh
	
	}
}



