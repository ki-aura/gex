#ifndef GEX_FILE_HANDLING_H
#define GEX_FILE_HANDLING_H

bool open_file(int argc, char *argv[]);
void close_file();
void save_changes();
void abandon_changes();
void insert_bytes();
void delete_bytes();
	

#endif
