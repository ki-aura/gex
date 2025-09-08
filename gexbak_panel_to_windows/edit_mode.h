#ifndef GEX_EDIT_MODE_H
#define GEX_EDIT_MODE_H

void init_view_mode();
void end_edit_mode(int k);
void e_handle_keys(int k);
void e_handle_full_grid_keys(int k);
void e_handle_partial_grid_keys(int k);
void e_copy_screen();
void e_refresh_hex();
void e_refresh_ascii();
void e_save_changes();
void e_build_grids_from_map_copy();
void e_handle_click(clickwin win, int row, int col);

#endif
