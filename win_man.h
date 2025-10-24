#ifndef GEX_WIN_MAN_H
#define GEX_WIN_MAN_H

void size_windows(void);
void create_windows(void);
void delete_windows(void);
void refresh_status(void);
void refresh_grids(void);
void update_all_windows(void);
void update_cursor(void);
unsigned long  popup_question(const char *qline1, const char *qline2, popup_types pt);
bool create_main_menu(void);


#endif
