#include "gex.h"
#include "edit_mode.h"

void e_handle_partial_grid_keys(int k)
{
	switch(k){
	// simple cases for home and end
	case KEY_HOME:
		// move start of current row
		hex.cur_col=1;
		hex.cur_digit=1;	// first hex digit (takes 3 spaces)
		hex.is_lnib = true;	// left nibble of that digit
		;break;

	case KEY_END:
		// move end of current row
		if(hex.cur_row < hex.max_row) {
			hex.cur_col = (hex.digits*3) - 2; // l nibble of last digit
			hex.cur_digit = hex.digits;	// first hex digit (takes 3 spaces)
			hex.is_lnib = true;	// left nibble of that digit
		} else {
			hex.cur_col = hex.max_col -2; // l nibble of last digit
			hex.cur_digit = hex.max_digit;	// first hex digit (takes 3 spaces)
			hex.is_lnib = true;	// left nibble of that digit
		}
		break;

	case KEY_LEFT:
		// 1 hex.digits = lnib rnib space
	if(hex.cur_row < hex.max_row) {
	// we're on a normal row.
		if(app.in_hex){
			if (!hex.is_lnib){
				// we can safely move to left nib. don't move ascii cursor
				hex.cur_col--;
				hex.is_lnib=true;
			} else {
				// we're on lef nib. move back 2 unless at start of row, 
				// otherwise wrap back to end of row
				if(hex.cur_digit > 1){
					hex.cur_col-=2;
					hex.cur_digit--;
					hex.is_lnib=false;
				} else {
					hex.cur_col=(hex.digits*3) - 1; //1 this time as go to right nibble
					hex.cur_digit=hex.digits;	// first hex digit (takes 3 spaces)
					hex.is_lnib = false;	// left nibble of that digit
				}
			}
		
		} else { // we're in ascii pane
			if(hex.cur_digit > 1){
				hex.cur_col-=3;
				hex.cur_digit--;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col=(hex.digits*3) - 2;
					hex.cur_digit=hex.digits;	// last hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}
	} else {
	// we're on a short row
		if(app.in_hex){
			if (!hex.is_lnib){
				// we can safely move to left nib. don't move ascii cursor
				hex.cur_col--;
				hex.is_lnib=true;
			} else {
				// we're on lef nib. move back 2 unless at start of row, 
				// otherwise wrap back to end of row
				if(hex.cur_digit > 1){
					hex.cur_col-=2;
					hex.cur_digit--;
					hex.is_lnib=false;
				} else {
					hex.cur_col = hex.max_col -1;  //1 this time as go to right nibble
					hex.cur_digit = hex.max_digit;	// first hex digit (takes 3 spaces)
					hex.is_lnib = false;	// left nibble of that digit
				}
			}
		
		} else { // we're in ascii pane
			if(hex.cur_digit > 1){
				hex.cur_col-=3;
				hex.cur_digit--;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col = (hex.digits*3) - 2; 
					hex.cur_digit = hex.max_digit;	// last hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}
	
	}
		break;
		
	case KEY_RIGHT:
		// 1 hex.digits = lnib rnib space
	if(hex.cur_row < hex.max_row) {
		if(app.in_hex){
			if (hex.is_lnib){
				// we can safely move to right nib. don't move ascii cursor
				hex.cur_col++;
				hex.is_lnib=false;
			} else {
				// we're on right nib. move 2 unless at end of row, 
				// otherwise wrap back to start of row
				if(hex.cur_digit < hex.digits){
					hex.cur_col+=2;
					hex.cur_digit++;
					hex.is_lnib=true;
				} else {
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
				}
			}		
		} else { // we're in ascii pane
			if(hex.cur_digit < hex.digits){
				hex.cur_col+=3;
				hex.cur_digit++;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}
	} else {
		if(app.in_hex){
			if (hex.is_lnib){
				// we can safely move to right nib. don't move ascii cursor
				hex.cur_col++;
				hex.is_lnib=false;
			} else {
				// we're on right nib. move 2 unless at end of row, 
				// otherwise wrap back to start of row
				if(hex.cur_digit < hex.max_digit){
					hex.cur_col+=2;
					hex.cur_digit++;
					hex.is_lnib=true;
				} else {
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
				}
			}		
		} else { // we're in ascii pane
			if(hex.cur_digit < hex.max_digit){
				hex.cur_col+=3;
				hex.cur_digit++;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}
	
	}
		break;
		
	case KEY_UP:
		if(hex.cur_row > 1) 
			hex.cur_row--;
		else  {
			// we have to go up further after max_col
			if(hex.cur_col <= hex.max_col)
				hex.cur_row = hex.max_row;
			else
				hex.cur_row = hex.max_row - 1;
		}
		break;
		
	case KEY_DOWN:
	// we can go down further at the start of the line
	if(hex.cur_col <= hex.max_col){
		if(hex.cur_row < hex.max_row)
			hex.cur_row++;
		else
			hex.cur_row = 1;
	} else {
		if(hex.cur_row < (hex.max_row - 1))
			hex.cur_row++;
		else
			hex.cur_row = 1;
	}	
		break;

	case KEY_NPAGE:
	if(hex.cur_col <= hex.max_col)
		hex.cur_row = hex.max_row;
	else
		hex.cur_row = hex.max_row -1;
		break;

	case KEY_PPAGE:
		hex.cur_row = 1;
		break;		
	
	}
}

void e_handle_full_grid_keys(int k)
{
	switch(k){
	// simple cases for home and end
	case KEY_HOME:
		// move start of current row
		hex.cur_col=1;
		hex.cur_digit=1;	// first hex digit (takes 3 spaces)
		hex.is_lnib = true;	// left nibble of that digit
		;break;

	case KEY_END:
		// move end of current row
		hex.cur_col = (hex.digits*3) - 2; // l nibble of last digit
		hex.cur_digit = hex.digits;	// first hex digit (takes 3 spaces)
		hex.is_lnib = true;	// left nibble of that digit
		break;

	case KEY_LEFT:
		// 1 hex.digits = lnib rnib space
		// boundary is 1 to hex.width -1
		if(app.in_hex){
			if (!hex.is_lnib){
				// we can safely move to left nib. don't move ascii cursor
				hex.cur_col--;
				hex.is_lnib=true;
			} else {
				// we're on lef nib. move back 2 unless at start of row, 
				// otherwise wrap back to end of row
				if(hex.cur_digit > 1){
					hex.cur_col-=2;
					hex.cur_digit--;
					hex.is_lnib=false;
				} else {
					hex.cur_col=(hex.digits*3) - 1; //1 this time as go to right nibble
					hex.cur_digit=hex.digits;	// first hex digit (takes 3 spaces)
					hex.is_lnib = false;	// left nibble of that digit
				}
			}
		
		} else { // we're in ascii pane
			if(hex.cur_digit > 1){
				hex.cur_col-=3;
				hex.cur_digit--;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col=(hex.digits*3) - 2;
					hex.cur_digit=hex.digits;	// last hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}

		break;
		
	case KEY_RIGHT:
		// 1 hex.digits = lnib rnib space
		// boundary is 1 to hex.width -1
		if(app.in_hex){
			if (hex.is_lnib){
				// we can safely move to right nib. don't move ascii cursor
				hex.cur_col++;
				hex.is_lnib=false;
			} else {
				// we're on right nib. move 2 unless at end of row, 
				// otherwise wrap back to start of row
				if(hex.cur_digit < hex.digits){
					hex.cur_col+=2;
					hex.cur_digit++;
					hex.is_lnib=true;
				} else {
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
				}
			}		
		} else { // we're in ascii pane
			if(hex.cur_digit < hex.digits){
				hex.cur_col+=3;
				hex.cur_digit++;
				hex.is_lnib = true;
			} else { // wrap col
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_lnib = true;	// left nibble of that digit
			}
		}
		break;
		
	case KEY_UP:
		if(hex.cur_row > 1) 
			hex.cur_row--;
		else 
			hex.cur_row = hex.rows;
		break;
		
	case KEY_DOWN:
		if(hex.cur_row < hex.rows)
			hex.cur_row++;
		else
			hex.cur_row = 1;
		break;

	case KEY_NPAGE:
		hex.cur_row = hex.rows;
		break;

	case KEY_PPAGE:
		hex.cur_row = 1;
		break;
	
	}
}

void e_handle_keys(int k)
{
	// hand off to handle any movement keys
	if(hex.map_copy_len == hex.grid){
		e_handle_full_grid_keys(k);
	} else {
		e_handle_partial_grid_keys(k);
	}
	
	// now continue with functional keys here
int idx;
unsigned char nibble;

	switch(k){
	// do stuff
	case KEY_MAC_ENTER:
		e_save_changes();
		break;
		
	case KEY_TAB:
		//ensure on left nib on ascii pane return
		if(!hex.is_lnib){
			hex.cur_col--;
			hex.is_lnib=true;
		}
		// flip panes
		app.in_hex = !app.in_hex;
		break; 
	default: 
		
	// handle non-movement - editing 
DP_ON=true;		
		// where are we in hex.gc_copy
		idx = (((hex.cur_row-1) * hex.digits) + (hex.cur_digit-1));
                if (!app.in_hex) { // we're in ascii pane
                    if (isprint(k)) {
                        hex.map_copy[idx] = (unsigned char)k;
                        hex.changes_made = true;
                        e_build_grids_from_map_copy();
                        e_refresh_ascii();
                        e_refresh_hex();
                        e_handle_keys(KEY_RIGHT);
                    }
                } else {  // we are in hex pane
                    if ((k >= '0' && k <= '9') || (k >= 'A' && k <= 'F') || (k >= 'a' && k <= 'f')) {
                        nibble = (k >= '0' && k <= '9') ? (k - '0') :
                                               (k >= 'a') ? (k - 'a' + 10) : (k - 'A' + 10);

                        if (hex.is_lnib) {
                            hex.map_copy[idx] = (hex.map_copy[idx] & 0x0F) | (nibble << 4);
                            hex.changes_made = true;
			e_build_grids_from_map_copy();
			e_refresh_ascii();
			e_refresh_hex();
			e_handle_keys(KEY_RIGHT);
                        } else {
                            hex.map_copy[idx] = (hex.map_copy[idx] & 0xF0) | nibble;
                            hex.changes_made = true;
			e_build_grids_from_map_copy();
			e_refresh_ascii();
			e_refresh_hex();
			e_handle_keys(KEY_RIGHT);
                        }
                    }
                }
		break;
	}

	// move the cursor
	if (app.in_hex) {
		wnoutrefresh(ascii.win);	
		wmove(hex.win, hex.cur_row, hex.cur_col);
		wnoutrefresh(hex.win);
	} else {
		wnoutrefresh(hex.win);
		wmove(ascii.win, hex.cur_row, hex.cur_digit);
		wnoutrefresh(ascii.win);	
	}
	doupdate();	
}

void init_edit_mode()
{
	// set the helper bar to something mode helpful
	refresh_status(); wrefresh(status.win);
	refresh_helper("Options: Escape Enter Tab"); wrefresh(helper.win);
	
	// grab a screen copy
	e_copy_screen();
	e_refresh_hex();
	e_refresh_ascii();
	doupdate();
	
	// set edit mode coming in defaults
	app.in_hex = true;	// start in hex screen
	hex.changes_made = false;
	// cursor starting location
	hex.cur_row=1;
	hex.cur_col=1;
	hex.cur_digit=1;	// first hex digit (takes 3 spaces)
	hex.is_lnib = true;	// left nibble of that digit
	
	// show cursor
	curs_set(2);
	wmove(hex.win, hex.cur_row, hex.cur_col);
	wrefresh(hex.win);
}

void end_edit_mode(int k)
{
	bool e_exit = false; // default is not to 
	
	// if screen resized then no option, otherwise ask
	if(k==KEY_RESIZE)
		e_exit = true;
	else
		if( popup_question("Are you sure you want to exit Edit mode?",
				   "All unsaved changes will be lost (y/n)", PTYPE_YN, EDIT_MODE) )
			e_exit = true;
		
	if (e_exit){
		// clean up
		curs_set(0);
		free(hex.gc_copy);
		free(ascii.gc_copy);
		free(hex.map_copy);
		
		// pass control back to main loop by setting mode back to view
		app.mode = VIEW_MODE;
	}	
}

void e_build_grids_from_map_copy()
{
	memset(hex.gc_copy, ' ', (hex.grid * 3));
	memset(ascii.gc_copy, ' ', (hex.grid));
	
	char t_hex[2];
	for(int i=0; i < (int)hex.map_copy_len; i++){
		byte_to_hex(hex.map_copy[i], t_hex);
		
		hex.gc_copy[i * 3] = t_hex[0];
		hex.gc_copy[(i * 3) +1] = t_hex[1];		
		
		ascii.gc_copy[i] = byte_to_ascii(hex.map_copy[i]);
	}
}


void e_copy_screen()
{
	// copy the segment of file relating to the grid
	// from hex.v_start to either + size of grid in bytes or end of file whichever is smallest
	hex.map_copy_len = hex.v_end - hex.v_start +1;
	
//	int max_row;	// this is the max row we can edit if screen > file size
//	int max_col; 	// this is the max col on the max row we can edit if screen > file size
//	int max_digit; // this is the max digit on the max row we can edit if screen > file size
	hex.max_row = hex.map_copy_len / hex.digits; // number of full rows
	if((hex.max_row * hex.digits) == hex.map_copy_len) {
		// we ended up with exactly a full row
		hex.max_digit = hex.digits;
		hex.max_col = hex.digits * 3;
	} else {
		// we ended up mid-row
		hex.max_digit = hex.map_copy_len - (hex.max_row * hex.digits);
		hex.max_col = hex.max_digit * 3;
		hex.max_row++; // we can go to next row, but only so far along
	}

//DP_ON=true; sprintf(tmp, "copy screen %i %lu", hex.map_copy_len, hex.v_start); DP(tmp); 
	
	// allocate space and copy from the app.map
	hex.map_copy = malloc(hex.map_copy_len + 1);
	memcpy(hex.map_copy, (app.map + hex.v_start), hex.map_copy_len);
	hex.map_copy[hex.map_copy_len] = '\0';

//DP_ON=true; sprintf(tmp, "copy map %c %c", hex.map_copy[0], hex.map_copy[1]); DP(tmp); 

hex.map_copy[0]='!';
hex.map_copy[1]='Y';
hex.map_copy[2]='e';
hex.map_copy[3]='s';
//DP_ON=true; sprintf(tmp, "copy map %c %c", hex.map_copy[0], hex.map_copy[1]); DP(tmp); 
	
	// now the displayable grid contents
	hex.gc_copy = malloc((hex.grid * 3) + 1);
	ascii.gc_copy = malloc(hex.grid + 1);

	e_build_grids_from_map_copy();
}


void e_refresh_hex()
{
	box(hex.win, 0, 0);

	int hr=1; // offset print row on grid. 1 avoids the borders
	int hc=1;
	int i=0; 
	int grid_points = hex.grid * 3;
	int row_points = hex.digits * 3;
	while(i<grid_points ){
		// print as much as a row as we can
		while ((i < grid_points) && (hc <= row_points)){
			mvwprintw(hex.win, hr, hc, "%c", hex.gc_copy[i]);
			hc++;
			i++;
		}
		hc = 1;
		hr++;
	}
	wnoutrefresh(hex.win);
}

void e_refresh_ascii()
{
	box(ascii.win, 0, 0);
	
	int hr=1; // offset print row on grid. 1 avoids the borders
	int hc=1;
	int i=0; 
	while(i<hex.grid){
		// print as much as a row as we can
		while ((i < hex.grid) && (hc <= hex.digits)){
			mvwprintw(ascii.win, hr, hc, "%c", ascii.gc_copy[i]);
			hc++;
			i++;
		}
		hc = 1;
		hr++;
	}
	wnoutrefresh(ascii.win);
}

void e_save_changes(){
	if (!hex.changes_made)
		popup_question("No changes made",
			"Press any key to continue", PTYPE_CONTINUE, EDIT_MODE);
	else if(popup_question("Are you sure you want to save changes?",
			"This action can not be undone (y/n)", PTYPE_YN, EDIT_MODE)){
		// save changes 
		
		hex.changes_made = FALSE;
	}
}



