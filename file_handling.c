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




