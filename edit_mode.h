#ifndef GEX_EDIT_MODE_H
#define GEX_EDIT_MODE_H

void init_edit_mode();
void end_edit_mode();
void e_handle_keys(int k);
void e_copy_screen();
void e_refresh_hex();
void e_refresh_ascii();
void e_save_changes();

#endif
