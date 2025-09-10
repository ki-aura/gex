#include "gex.h"
#include "file_handling.h"

bool open_file(int argc, char *argv[])
{

	// check we have a file name
	if (argc != 2) {
	fputs("Usage: %s <file>\n", stderr);
		return false;
	} else {
		app.fname = argv[1];
	}

	// try to open it
	app.fd = open(argv[1], O_RDWR);
	if (app.fd < 0) {
		return false;
	}

	// get file stats
	if (fstat(app.fd, &app.fs) < 0) {
		close(app.fd);
		return false;
	}

	// check file not empty
	if (app.fs.st_size < 10) {
		close(app.fd);
		return false;	
	} else {
		app.fsize = app.fs.st_size;
	}

	app.map = mmap(NULL, app.fsize, PROT_READ | PROT_WRITE, MAP_SHARED, app.fd, 0);
	if (app.map == MAP_FAILED) {
		close(app.fd);
		return false;	
	}

	return true;
}

void close_file()
{
	munmap(app.map, app.fsize);
	close(app.fd);
}

// Comparison for qsort: by int key
typedef struct {
	int key; 
	char val; 
} kv_t;

int cmp_key(const void *a, const void *b) {
    const kv_t *pa = (const kv_t*)a;
    const kv_t *pb = (const kv_t*)b;
    return pa->key - pb->key;
}


void save_changes(){
	if (kh_size(app.edmap) == 0)
		popup_question("No changes made",
			"Press any key to continue", PTYPE_CONTINUE);
	else if(popup_question("Are you sure you want to save changes?",
			"This action can not be undone (y/n)", PTYPE_YN)){
	
		// save changes 
		for (slot = kh_begin(app.edmap); slot != kh_end(app.edmap); slot++) {
			if (kh_exist(app.edmap, slot)) {
/*			size_t i = kh_key(app.edmap, slot);
                int v = kh_val(app.edmap, slot);
			
			snprintf(tmp,40,"o: %lu b: %i",i, v);
			popup_question(tmp, "", PTYPE_CONTINUE);
 */
            app.map[kh_key(app.edmap, slot)] = kh_val(app.edmap, slot);
			}
		}
		
		// and sync it out
		msync(app.map, app.fsize, MS_SYNC);
		// clear change history as these are now permanent
		kh_clear(charmap, app.edmap);
		// refresh to get rid of old change highlights
		update_all_windows();
		handle_global_keys(KEY_REFRESH);
	}	
}

void abandon_changes(){
    if (kh_size(app.edmap) == 0)
        popup_question("No changes to abandon",
            "Press any key to continue", PTYPE_CONTINUE);
    else if(popup_question("Are you sure you want to abandon changes?",
            "This action can not be undone (y/n)", PTYPE_YN)){
    
        // abandon changes
        kh_clear(charmap, app.edmap);
        // refresh to get rid of old change highlights
        update_all_windows();
        handle_global_keys(KEY_REFRESH);
    }
}





