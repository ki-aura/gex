#include "gex.h"
#include "file_handling.h"

bool open_file(int argc, char *argv[])
{

	// check we have a file name
	if (argc != 2) {
	fputs("Usage: %s <file>\n", stderr);
		DP("no filename given");
		return false;
	} else {
		app.fname = argv[1];
	}

	// try to open it
	app.fd = open(argv[1], O_RDWR);
	if (app.fd < 0) {
		DP("file open");
		return false;
	}

	// get file stats
	if (fstat(app.fd, &app.fs) < 0) {
		DP("fstat fail");
		close(app.fd);
		return false;
	}

	// check file not empty
	if (app.fs.st_size < 10) {
		DP("empty file");
		close(app.fd);
		return false;	
	} else {
		app.fsize = app.fs.st_size;
	}

	app.map = mmap(NULL, app.fsize, PROT_READ | PROT_WRITE, MAP_SHARED, app.fd, 0);
	if (app.map == MAP_FAILED) {
		DP("mmap failed");
		close(app.fd);
		return false;	
	}

	return true;
}

void close_file()
{

	if (munmap(app.map, app.fsize) < 0) {
		DP("munmap error");
	}
	close(app.fd);

}



