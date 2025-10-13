#ifndef GTREE_H
#define GTREE_H  

#include <stdbool.h>
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir (POSIX)


// Fallback maximum path length if PATH_MAX is not defined by the system
#ifndef PATH_MAX
#define PATH_MAX 1024   
#endif

#define TH_VERSION "1.1.0"


#endif