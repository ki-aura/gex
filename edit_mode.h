#ifndef GEX_EDIT_MODE_H
#define GEX_EDIT_MODE_H

void e_init_view_mode();
void e_end_edit_mode(int k);
void e_copy_screen();
void e_refresh_hex();
void e_refresh_ascii();
void e_save_changes();
void e_build_grids_from_map_copy();

#endif
