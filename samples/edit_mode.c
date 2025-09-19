#include "gex.h"


void rrefresh_hex() {

///////////// CHECK E_REFRESH FOR CHANGE HANDLING
	char hinib, lonib;
	int hr=0, hc=0, i=0; 
	
	int grid_points = hex.grid * 3; // 3 chars per hex byte; hi lo space
	while((i<grid_points) && ((hex.v_start+i) < app.fsize)){
		// print as much as a row as we can
		while ((i < grid_points) && (hc < hex.width) && ((hex.v_start+i) < app.fsize)){
			byte_to_nibs(app.map[hex.v_start + i], &hinib, &lonib);			
			mvwprintw(hex.win, hr, hc, "%c", hinib);
			mvwprintw(hex.win, hr, hc+1, "%c", lonib);
			hc+=3; // 2 nibbles and a space
			i++;	// next byte
		}
		hc = 0;
		hr++;
	}
	box(hex.border, 0, 0);
	wnoutrefresh(hex.border);
	wnoutrefresh(hex.win);
}

void rrefresh_ascii() {
///////////// CHECK E_REFRESH FOR CHANGE HANDLING
	int ar=0, ac=0, i=0; 
	
	while((i<hex.grid) && ((hex.v_start+i) < app.fsize)){
		// print as much as a row as we can
		while ((i < hex.grid) && (ac < ascii.width) && ((hex.v_start+i) < app.fsize)){
			mvwprintw(ascii.win, ar, ac, "%c", byte_to_ascii(app.map[hex.v_start + i]));
			ac++;
			i++;
		}
		ac = 0;
		ar++;
	}
	box(ascii.border, 0, 0);
	wnoutrefresh(ascii.border);
	wnoutrefresh(ascii.win);
}



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

//////////////////////////////////////////////////////////////////////


void e_handle_keys(int k){
int idx;
unsigned char nibble;
bool undo = false;
	

	// now continue with functional keys here
	switch(k){
	// do stuff
	case KEY_MAC_ENTER:
		e_save_changes();
		break;
	
//int ascii_rc_to_offset(int row, int col);
//int hex_rc_to_offset(int row, int col);
	
	default: 	
		// handle non-movement - editing 		
		// where are we in hex.gc_copy
		//idx = ascii_rc_to_offset(hex.cur_row, ascii.cur_col);
//		(((hex.cur_row-1) * ascii.width) + (hex.cur_digit-1));
		//return (row * ascii.width) + digit;
	
		if (undo){
			// backspace was pressed
			// check if there's a real change at this point, or we've typed what was orignally there
			slot = kh_get(charmap, app.edmap, (size_t)(app.map + hex.v_start + idx));
			if (slot != kh_end(app.edmap)) {
				// set the bit back to the original map
				// remove the undo map 
				kh_del(charmap, app.edmap, slot);
				e_build_grids_from_map_copy();
				e_refresh_ascii();
				e_refresh_hex();
			}
		} else {	
			// normal edit processing (not undo)	
			idx = row_digit_to_offset(hex.cur_row, hex.cur_digit);
			if (!app.in_hex) { // we're in ascii pane
			    if (isprint(k)) {
				// if it's the same as the underlying file then get rid of any edit map
				if( k == app.map[hex.v_start + idx]){
					slot = kh_get(charmap, app.edmap, (size_t)(hex.v_start + idx));
					if (slot != kh_end(app.edmap)) 
						kh_del(charmap, app.edmap, slot);
					}
				else {  // push the change onto the edit map
					slot = kh_put(charmap, app.edmap, (size_t)(hex.v_start + idx), &khret);
					kh_val(app.edmap, slot) = (unsigned char)k;
				}
				// Update and move
				update_all_windows();
				handle_in_screen_movement(KEY_RIGHT);
			    }
			} else {  // we are in hex pane
				if ((k >= '0' && k <= '9') || (k >= 'A' && k <= 'F') || (k >= 'a' && k <= 'f')) {
					nibble = (k >= '0' && k <= '9') ? (k - '0') :
						       (k >= 'a') ? (k - 'a' + 10) : (k - 'A' + 10);
	
					if (hex.is_hinib) {
						// push the change onto the edit map
						slot = kh_put(charmap, app.edmap, (size_t)(hex.v_start + idx), &khret);
						kh_val(app.edmap, slot) = (unsigned char)((app.map[hex.v_start + idx] & 0x0F) | (nibble << 4));
					} else {
						// push the change onto the edit map
						slot = kh_put(charmap, app.edmap, (size_t)(hex.v_start + idx), &khret);
						kh_val(app.edmap, slot) = (unsigned char)((app.map[hex.v_start + idx] & 0xF0) | nibble);
					}
				// Update and move
				update_all_windows();
				handle_in_screen_movement(KEY_RIGHT);
				}
			}
		}
		break;
	}


}

void vvv_handle_keys(int k){

}

void goto_byte(){
 
}
void e_handle_full_grid_keys(int k){
}
void e_handle_partial_grid_keys(int k){
}





