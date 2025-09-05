#ifndef GEX_VIEW_MODE_H
#define GEX_VIEW_MODE_H

void v_handle_keys(int k);
void v_populate_grids();
void v_refresh_ascii();
void v_refresh_hex();
void v_goto_byte();
void v_update_all_windows();
void size_windows();
void create_windows();
void delete_windows();

#endif
