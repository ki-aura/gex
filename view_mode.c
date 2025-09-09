#include "gex.h"


void vvv_handle_keys(int k)
{
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



