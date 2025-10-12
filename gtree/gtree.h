#ifndef GTREE_H
#define GTREE_H  

#include <stdbool.h>
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir (POSIX)


// Fallback maximum path length if PATH_MAX is not defined by the system
#ifndef PATH_MAX
#define PATH_MAX 1024   
#endif

#define TH_VERSION "1.1.0"
// Defines the maximum depth and also the size of the explicit stack array
#define MAX_DEPTH 1024   

// -----------------------------------------------------
// ------------------ Definitions ------------------
// -----------------------------------------------------

// Node structure to hold subdirectory info in a linked list.
// Used to store subdirectories discovered in a directory *before* traversing them.
// This decouples the scanning phase from the descending phase.
typedef struct SubDirNode {
    char path[PATH_MAX];       // Full path of the subdirectory (e.g., "/home/user/dir/subdir")
    bool is_symlink;           // True if this directory entry itself is a symbolic link
    char sym_path[PATH_MAX];   // Target path if symlink (e.g., "../../otherdir")
    struct SubDirNode *next;   // Pointer to next subdirectory (linked list for children)
} SubDirNode;

// Frame structure representing one directory level in the explicit stack.
// This structure replaces the 'stack frame' of a recursive function call.
typedef struct DirFrame {
    char path[PATH_MAX];         // Path of this directory
    DIR *dir;                    // DIR* stream for reading entries with readdir
    SubDirNode *subdirs;         // Head of the linked list of subdirectories found (Phase 1 result)
    SubDirNode *current;         // Pointer to the current subdir being processed (iterator for Phase 2)
    int depth;                   // Depth in the directory tree (0 = starting directory)
    size_t dir_file_count;       // Number of regular files in this specific directory
    off_t dir_file_size;         // Cumulative size of regular files in this specific directory
    bool has_sibling[MAX_DEPTH+1]; // Used to track tree branches for formatted output (│/└/├)
    bool is_last;                // True if this directory is the last among its siblings (for print formatting)
} DirFrame;

// -------------------------------- Final Report -------------------------------
// Holds summary stats accumulated during the traversal.
typedef struct ActivityReport {
	size_t TOTAL_file_count;           // Total number of regular files
	size_t TOTAL_linked_files;         // Number of regular files that are symbolic links
	off_t TOTAL_file_size;             // Total size of all regular files
	size_t TOTAL_directories;          // Total directories successfully traversed
	size_t TOTAL_linked_directories;   // Symlinked directories encountered
} ActivityReport;

#endif