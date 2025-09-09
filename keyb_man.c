#include "gex.h"

void handle_click(clickwin win, int row, int col){
	// if we clicked outside hex or ascii windows then don't care
	if(win == WIN_OTHER) return;

	// handle_full_grid_clicks 		
	if(win == WIN_HEX) {
		// this so far only handles full panes
		app.in_hex = true;
		hex.cur_row = row;
		hex.cur_digit = col/3;
		hex.cur_col = col; 
		// move back 1 if we clicked on a space in the grid
		if((hex.cur_col%3)==2)hex.cur_col--;
		// work out which nibble we're on
		if((hex.cur_col%3)==0)hex.is_hinib=true; else hex.is_hinib=false;
	}
	
	if(win == WIN_ASCII){
		// this so far only handles full panes
		app.in_hex = false;
		hex.cur_row = row;
		hex.cur_digit = col;
		hex.cur_col = (col *3);		
		hex.is_hinib=true;
//snprintf(tmp,200,"(A) col %d digit %d row %d HiNib %d",hex.cur_col, hex.cur_digit, row, (int)hex.is_hinib); popup_question(tmp, "", PTYPE_CONTINUE);
	}

/*	// boundary checks needed for and partially filled window
	if(hex.map_copy_len != hex.grid) {
		// if we clicked above the max row set to max row
		if(hex.cur_row > hex.max_row) hex.cur_row = hex.max_row;
		// if we are on max_row and further over than max col, set to max col
		if ((hex.cur_row == hex.max_row) && (hex.cur_col > hex.max_col)) {
			hex.cur_col = hex.max_col-1;
			hex.cur_digit = (hex.max_col+2)/3;
			}
	} 
*/
	//e_handle_keys(KEY_HELP); // a key that will drop through to the display updates
	return;

}
/*




				switching between windows is bugged. need consistent col mapping
				review how hex_cur_col and cur_digit are used as it's getting
				messed up somewhere. espec on resize
				consider one set of app. coordinates and a
				from-ascii mapping back to app... and app... to hex. i.e. maintain
				hex as the master for app & translate ascii includeing mouse clicks 
				(i.e) why need a back mapping. don't forget nibble placement as well. 
				helper functions
					app.byte offset  -> hwin.rc
					"				-> ascii.rc
					ascii.rc 		-> appbyte offset
					hex ditto
					
				** implementation note **
				drop short row logic for moving around. just check when non-movement
				keys imply edit whether ths is valid and ignor if not
				
				implement menu to replace esc to exit - i.e. esc to menu
					quit, goto byte, save changes, abandon changes, delete (from cursor, num bytes)
					insert (from cursor numb bytes)
				
				BUG - keys are taking 2 presses to update the debug info in helper 
				window - could be symptomatic of more. review main ch logic and 
				consider the non-waiting version of ch - more research needed on that
				though - probs last option
				
				impleny F keys for goto first change, last and next (needs sort)
				F1 for help
					
					









*/
void k_left(){
	if(app.in_hex){
		if (!hex.is_hinib){
			// we can safely move to left nib. don't move ascii cursor
			hex.cur_col--;
			hex.is_hinib=true;
		} else {
			// we're on lef nib. move back 2 unless at start of row, 
			// otherwise wrap back to end of row
			if(hex.cur_digit > 0){
				hex.cur_col-=2;
				hex.cur_digit--;
				hex.is_hinib=false;
			} else {
				hex.cur_col=(ascii.width*3) - 2; //this time as go to hi nibble
				hex.cur_digit=ascii.width-1;	// first hex digit (takes 3 spaces)
				hex.is_hinib = false;	// left nibble of that digit
			}
		}
	} else { // we're in ascii pane
		if(hex.cur_digit > 0){
			hex.cur_col-=3;
			hex.cur_digit--;
			hex.is_hinib = true;
		} else { // wrap col
				hex.cur_col=(ascii.width*3);
				hex.cur_digit=ascii.width-1;	// last hex digit (takes 3 spaces)
				hex.is_hinib = true;	// left nibble of that digit
		}
	}
}
void k_right(){}
void k_up(){}
void k_down(){}

void handle_in_screen_movement(int k){
app.lastkey = k;

	switch (k){
	case KEY_TAB:
	//ensure on left nib on ascii pane return
		if(!hex.is_hinib){
			hex.cur_col--;
			hex.is_hinib=true;
		}
		// flip panes
		app.in_hex = !app.in_hex;
		update_cursor();
		break; 
	
	case KEY_NCURSES_BACKSPACE:
	case KEY_MAC_DELETE:
	case KEY_LEFT:
		// do key left
		k_left();
		// maybe undo
		if(k != KEY_LEFT){
			// check hash table - if there, delete it
		}
		update_cursor();
		break;
		
	case KEY_RIGHT:
		break;

	case KEY_HOME:
		hex.cur_col = 0;
		hex.cur_digit = 0;
		hex.cur_row = 0;
		update_cursor();
		break;
		
	case KEY_END:
		hex.cur_col = hex.width - 3;
		hex.cur_digit = ascii.width-1;
		hex.cur_row = hex.height-1;
		update_cursor();
		break;

	default:
		update_cursor();
		break;		
	}
}

void handle_scrolling_movement(int k){

	switch (k){
	case KEY_UP:
	case KEY_DOWN:
	case KEY_NPAGE:
	case KEY_PPAGE:
	//case MENU_GOTO:
	default:
		update_cursor();
		break;

	}
}

void handle_edit_keys(int k){
	switch (k){
	case KEY_UP:
	case KEY_DOWN:
	case KEY_NPAGE:
	case KEY_PPAGE:
	//case MENU_GOTO:
	default:
		update_cursor();
		break;

	}

}

void e_handle_partial_grid_keys(int k){
	switch(k){
	// simple cases for home and end
	case KEY_END:
		// move end of current row
		if(hex.cur_row < hex.max_row) {
			hex.cur_col = (ascii.width*3) - 2; // l nibble of last digit
			hex.cur_digit = ascii.width;	// first hex digit (takes 3 spaces)
			hex.is_hinib = true;	// left nibble of that digit
		} else {
			hex.cur_col = hex.max_col -2; // l nibble of last digit
			hex.cur_digit = hex.max_digit;	// first hex digit (takes 3 spaces)
			hex.is_hinib = true;	// left nibble of that digit
		}
		break;

	case KEY_LEFT:
		// 1 ascii.width = lnib rnib space
	if(hex.cur_row < hex.max_row) {
	// we're on a normal row.
		if(app.in_hex){
			if (!hex.is_hinib){
				// we can safely move to left nib. don't move ascii cursor
				hex.cur_col--;
				hex.is_hinib=true;
			} else {
				// we're on lef nib. move back 2 unless at start of row, 
				// otherwise wrap back to end of row
				if(hex.cur_digit > 1){
					hex.cur_col-=2;
					hex.cur_digit--;
					hex.is_hinib=false;
				} else {
					hex.cur_col=(ascii.width*3) - 1; //1 this time as go to right nibble
					hex.cur_digit=ascii.width;	// first hex digit (takes 3 spaces)
					hex.is_hinib = false;	// left nibble of that digit
				}
			}
		
		} else { // we're in ascii pane
			if(hex.cur_digit > 1){
				hex.cur_col-=3;
				hex.cur_digit--;
				hex.is_hinib = true;
			} else { // wrap col
					hex.cur_col=(ascii.width*3) - 2;
					hex.cur_digit=ascii.width;	// last hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
			}
		}
	} else {
	// we're on a short row
		if(app.in_hex){
			if (!hex.is_hinib){
				// we can safely move to left nib. don't move ascii cursor
				hex.cur_col--;
				hex.is_hinib=true;
			} else {
				// we're on lef nib. move back 2 unless at start of row, 
				// otherwise wrap back to end of row
				if(hex.cur_digit > 1){
					hex.cur_col-=2;
					hex.cur_digit--;
					hex.is_hinib=false;
				} else {
					hex.cur_col = hex.max_col -1;  //1 this time as go to right nibble
					hex.cur_digit = hex.max_digit;	// first hex digit (takes 3 spaces)
					hex.is_hinib = false;	// left nibble of that digit
				}
			}
		
		} else { // we're in ascii pane
			if(hex.cur_digit > 1){
				hex.cur_col-=3;
				hex.cur_digit--;
				hex.is_hinib = true;
			} else { // wrap col
					hex.cur_col = (ascii.width*3) - 2; 
					hex.cur_digit = hex.max_digit;	// last hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
			}
		}
	
	}
		break;
		
	case KEY_RIGHT:
		// 1 ascii.width = lnib rnib space
	if(hex.cur_row < hex.max_row) {
		if(app.in_hex){
			if (hex.is_hinib){
				// we can safely move to right nib. don't move ascii cursor
				hex.cur_col++;
				hex.is_hinib=false;
			} else {
				// we're on right nib. move 2 unless at end of row, 
				// otherwise wrap back to start of row
				if(hex.cur_digit < ascii.width){
					hex.cur_col+=2;
					hex.cur_digit++;
					hex.is_hinib=true;
				} else {
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
				}
			}		
		} else { // we're in ascii pane
			if(hex.cur_digit < ascii.width){
				hex.cur_col+=3;
				hex.cur_digit++;
				hex.is_hinib = true;
			} else { // wrap col
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
			}
		}
	} else {
		if(app.in_hex){
			if (hex.is_hinib){
				// we can safely move to right nib. don't move ascii cursor
				hex.cur_col++;
				hex.is_hinib=false;
			} else {
				// we're on right nib. move 2 unless at end of row, 
				// otherwise wrap back to start of row
				if(hex.cur_digit < hex.max_digit){
					hex.cur_col+=2;
					hex.cur_digit++;
					hex.is_hinib=true;
				} else {
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
				}
			}		
		} else { // we're in ascii pane
			if(hex.cur_digit < hex.max_digit){
				hex.cur_col+=3;
				hex.cur_digit++;
				hex.is_hinib = true;
			} else { // wrap col
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
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
	}
}

void e_handle_full_grid_keys(int k){
	switch(k){
	// simple cases for home and end

	case KEY_END:
		// move end of current row
		hex.cur_col = (ascii.width*3) - 2; // l nibble of last digit
		hex.cur_digit = ascii.width;	// first hex digit (takes 3 spaces)
		hex.is_hinib = true;	// left nibble of that digit
		break;

	case KEY_LEFT:
		// 1 ascii.width = lnib rnib space
		// boundary is 1 to hex.width -1
		if(app.in_hex){
			if (!hex.is_hinib){
				// we can safely move to left nib. don't move ascii cursor
				hex.cur_col--;
				hex.is_hinib=true;
			} else {
				// we're on lef nib. move back 2 unless at start of row, 
				// otherwise wrap back to end of row
				if(hex.cur_digit > 1){
					hex.cur_col-=2;
					hex.cur_digit--;
					hex.is_hinib=false;
				} else {
					hex.cur_col=(ascii.width*3) - 1; //1 this time as go to right nibble
					hex.cur_digit=ascii.width;	// first hex digit (takes 3 spaces)
					hex.is_hinib = false;	// left nibble of that digit
				}
			}
		
		} else { // we're in ascii pane
			if(hex.cur_digit > 1){
				hex.cur_col-=3;
				hex.cur_digit--;
				hex.is_hinib = true;
			} else { // wrap col
					hex.cur_col=(ascii.width*3) - 2;
					hex.cur_digit=ascii.width;	// last hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
			}
		}

		break;
		
	case KEY_RIGHT:
		// 1 ascii.width = lnib rnib space
		// boundary is 1 to hex.width -1
		if(app.in_hex){
			if (hex.is_hinib){
				// we can safely move to right nib. don't move ascii cursor
				hex.cur_col++;
				hex.is_hinib=false;
			} else {
				// we're on right nib. move 2 unless at end of row, 
				// otherwise wrap back to start of row
				if(hex.cur_digit < ascii.width){
					hex.cur_col+=2;
					hex.cur_digit++;
					hex.is_hinib=true;
				} else {
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
				}
			}		
		} else { // we're in ascii pane
			if(hex.cur_digit < ascii.width){
				hex.cur_col+=3;
				hex.cur_digit++;
				hex.is_hinib = true;
			} else { // wrap col
					hex.cur_col=1;
					hex.cur_digit=1;	// first hex digit (takes 3 spaces)
					hex.is_hinib = true;	// left nibble of that digit
			}
		}
		break;
		
	case KEY_UP:
		if(hex.cur_row > 1) 
			hex.cur_row--;
		else 
			hex.cur_row = hex.height;
		break;
		
	case KEY_DOWN:
		if(hex.cur_row < hex.height)
			hex.cur_row++;
		else
			hex.cur_row = 1;
		break;

	case KEY_NPAGE:
		hex.cur_row = hex.height;
		break;
	
	}
}

void e_handle_keys(int k){
int idx;
unsigned char nibble;
bool undo = false;
	
	// handle ay key movements that are common to both full and partial grids
	switch(k){
	case KEY_HOME:
	// move start of current row
		hex.cur_col=1;
		hex.cur_digit=1;	// first hex digit (takes 3 spaces)
		hex.is_hinib = true;	// left nibble of that digit
		;break;

	case KEY_PPAGE:
		hex.cur_row = 1;
		break;

	case KEY_TAB:
	//ensure on left nib on ascii pane return
		if(!hex.is_hinib){
			hex.cur_col--;
			hex.is_hinib=true;
		}
		// flip panes
		app.in_hex = !app.in_hex;
		break; 

	case KEY_NCURSES_BACKSPACE:
	case KEY_MAC_DELETE:
		// first move left 
		e_handle_keys(KEY_LEFT);
		// now set undo flag for any changes on this char or nibble 
		undo = true;
		// and trigger the delete
//		e_handle_keys(KEY_LEFT_PROXY); // this won't be handled, so will trigger default
		
	} // don't need a default option here as we continue below
	
	// hand off to handle any movement keys that differ wth a partial grid
	if(hex.map_copy_len == hex.grid){
		e_handle_full_grid_keys(k);
	} else {
		e_handle_partial_grid_keys(k);
	}
	
	// now continue with functional keys here
	switch(k){
	// do stuff
	case KEY_MAC_ENTER:
		e_save_changes();
		break;
		
	default: 	// handle non-movement - editing 		
		// where are we in hex.gc_copy
		idx = (((hex.cur_row-1) * ascii.width) + (hex.cur_digit-1));
		
		if (undo){
			// backspace was pressed
			// check if there's a real change at this point, or we've typed what was orignally there
			slot = kh_get(charmap, app.edmap, (int64_t)(app.map + hex.v_start + idx));
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
			if (!app.in_hex) { // we're in ascii pane
			    if (isprint(k)) {
				// if it's the same as the underlying file then get rid of any edit map
				if( k == app.map[hex.v_start + idx]){
					slot = kh_get(charmap, app.edmap, (int64_t)(app.map + hex.v_start + idx));
					if (slot != kh_end(app.edmap)) 
						kh_del(charmap, app.edmap, slot);
					}
				else {  // push the change onto the edit map
					slot = kh_put(charmap, app.edmap, (int64_t)(app.map + hex.v_start + idx), &khret);
					//if(khret) kh_val(app.edmap, slot).old_ch = (unsigned char)hex.map_copy[idx];
					kh_val(app.edmap, slot) = (unsigned char)k;
				}
				// Update the display only map
				
				// trigger a refresh
				e_build_grids_from_map_copy();
				e_refresh_ascii();
				e_refresh_hex();
				e_handle_keys(KEY_RIGHT);
			    }
			} else {  // we are in hex pane
				if ((k >= '0' && k <= '9') || (k >= 'A' && k <= 'F') || (k >= 'a' && k <= 'f')) {
					nibble = (k >= '0' && k <= '9') ? (k - '0') :
						       (k >= 'a') ? (k - 'a' + 10) : (k - 'A' + 10);
	
					if (hex.is_hinib) {
						// push the change onto the edit map
						slot = kh_put(charmap, app.edmap, (int64_t)(app.map + hex.v_start + idx), &khret);
						//if(khret) kh_val(app.edmap, slot).old_ch = (unsigned char)hex.map_copy[idx];
						kh_val(app.edmap, slot) = (unsigned char)((app.map[hex.v_start + idx] & 0x0F) | (nibble << 4));
						// Update the display only map
						//hex.map_copy[idx] = (hex.map_copy[idx] & 0x0F) | (nibble << 4);
					
						// trigger a refresh
						e_build_grids_from_map_copy();
						e_refresh_ascii();
						e_refresh_hex();
						e_handle_keys(KEY_RIGHT);
					} else {
						// push the change onto the edit map
						slot = kh_put(charmap, app.edmap, (int64_t)(app.map + hex.v_start + idx), &khret);
						//if(khret) kh_val(app.edmap, slot).old_ch = (unsigned char)hex.map_copy[idx];
						kh_val(app.edmap, slot) = (unsigned char)((app.map[hex.v_start + idx] & 0xF0) | nibble);
						// Update the display only map
						//hex.map_copy[idx] = (hex.map_copy[idx] & 0xF0) | nibble;
						// show changes
						e_build_grids_from_map_copy();
						e_refresh_ascii();
						e_refresh_hex();
						e_handle_keys(KEY_RIGHT);
					}
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

void vvv_handle_keys(int k){
//////DP("in move mode");
////DP_ON = false;

	switch(k){
	// simple cases for home and end
	case KEY_HOME:
		////DP("home");
		hex.v_start = 0;
		;break;
	case KEY_END:
		//DP("end");
		hex.v_start = app.fsize - hex.grid ;
		break;
	// going down file, add here an rely on boundary conditions to check of we're over
	case KEY_RIGHT:
		//DP("down l char");
		hex.v_start++;
		break;
	case KEY_DOWN:
		hex.v_start += ascii.width;
		break;
	case KEY_NPAGE:
		hex.v_start += hex.grid;
		break;
	// going up the file, we can't check of < 0 because we're working with unsigned ints
	// so we have to ensure we don't go below zero and end up massive
	case KEY_LEFT:
	// up on char
		if (hex.v_start > 0) 
			hex.v_start--;
		break;
	case KEY_UP:
	// up one line
		if(hex.v_start > (unsigned long)ascii.width)
			hex.v_start -= ascii.width;
		else
			hex.v_start = 0;
		break;
	// up one full grid
	case KEY_PPAGE:
		if(hex.v_start > (unsigned long)hex.grid)
			hex.v_start -= hex.grid;
		else
			hex.v_start = 0;
		break;
	}

	// boundary conditions
	if ((hex.v_start >= app.fsize) ||
			((hex.v_start + hex.grid) >= app.fsize))
		hex.v_start = app.fsize - hex.grid;
	
	if (app.fsize <= (unsigned long)hex.grid) 
		hex.v_start = 0;
	
	// calc v_end
	hex.v_end = hex.v_start + hex.grid -1;
	if (hex.v_end >= app.fsize) hex.v_end = app.fsize -1;
}

void goto_byte(){
        snprintf(tmp, 60, "Goto Byte? (0-%lu)", (unsigned long)app.fsize);
        // hex.v_start = will either be a new valid value or 0
        hex.v_start = popup_question(tmp, "", PTYPE_UNSIGNED_LONG);
        update_all_windows();
}

