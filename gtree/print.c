#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir (POSIX)
#include <string.h>     // For strncpy, strcmp, strrchr, memset, memcpy, snprintf
#include <sys/stat.h>   // For struct stat, lstat, stat, S_ISDIR, S_ISLNK (POSIX)
#include <libgen.h>     // For basename if needed (not used here)
#include <unistd.h>     // For readlink (POSIX)
#include <inttypes.h>   // For intmax_t
#include "gtree.h"
#include "memsafe.h"
#include "print.h"


// ----------------- Human readable file size -------------------
// Converts a size in bytes (off_t) to a human-readable string (e.g., 4.5K, 2.1M).
void human_size(off_t bytes, char *out, size_t outsz){
    const char *units[] = {"B", "K", "M", "G", "T"};
    double size = (double)bytes;
    int u = 0;
    // Loop while size is >= 1024 and we have a unit to move up to
    while (size >= 1024.0 && u < 4) {
        size /= 1024.0;
        u++;
    }
    // Format the output string with 1 decimal place and the unit
    snprintf(out, outsz, "%.1f%s", size, units[u]);
}

// ----------------- Printing helpers -------------------
static void print_tree_prefix(const DirFrame *frame)
{
    if (!frame) return;
    for (int i = 1; i < frame->depth; i++)
        printf("%s", frame->ancestor_siblings[i] ? "│   " : "    ");
}

static void print_directory_content(const char *name, bool is_symdir,
                                    const char *symPath, bool is_recursive,
                                    bool show_stats, size_t fc, off_t fs)
{
    if (is_symdir) {
        printf("@%s -> %s%s\n", name, symPath,
               is_recursive ? " [recursive - not followed]" : "");
        return;
    }

    if (show_stats && fc > 0) {
        char hsize[32];
        human_size(fs, hsize, sizeof(hsize));
        printf("%s [Files: %zu] [Size: %s]%s\n", name, fc, hsize,
               is_recursive ? " [recursive - not followed]" : "");
    } else {
        printf("%s%s\n", name,
               is_recursive ? " [recursive - not followed]" : "");
    }
}

// ----------------- Unified entry printing -------------------
// entry_name: for files this is the printable string (e.g., "@link -> target" or "filename"),
//             for directories pass NULL to print the directory's basename.
// is_dir: true => print directory (connector + stats/symlink handling)
//         false => print file (no connector, prints "    : filename" style as original)
void print_entry_line(const DirFrame *frame, bool is_last, bool is_symdir, 
					  const char *symPath, bool is_recursive, bool show_stats, 
					  const char *entry_name, bool is_dir) {
					   
    const char *basePath = frame ? frame->path : "";
    int depth = frame ? frame->depth : 0;
    const bool *ancestor_siblings = frame ? frame->ancestor_siblings : NULL;
    size_t fc = frame ? frame->dir_file_count : 0;
    off_t fs = frame ? frame->dir_file_size : 0;

    const char *slash = strrchr(basePath, '/');
    const char *dir_name = (slash && depth > 0) ? slash + 1 : basePath;

    // Print tree prefix (│   / spaces)
    if (ancestor_siblings)
        print_tree_prefix(frame);

    // --- FILE case: keep the original "    : name" behaviour (no connector) ---
    if (!is_dir) {
        // preserve original formatting: depth==0 ? "" : "    "
        printf("%s : %s\n", depth == 0 ? "" : "    ", entry_name ? entry_name : "");
        return;
    }

    // --- DIRECTORY case: print connector + directory content (or symlink) ---
    if (depth > 0)
        printf("%s── ", is_last ? "└" : "├");

    // Use dir_name as the printed name for directories
    print_directory_content(dir_name, is_symdir, symPath, is_recursive, show_stats, fc, fs);
}

static void add_subfile(bool is_symlink, char *fname, SubDirFile **tail_ptr){
	// allocate a new node & populate it
	SubDirFile *n = xmalloc(sizeof(SubDirFile));
	snprintf(n->name, PATH_MAX, "%s", fname);
	n->is_symlink = is_symlink;
	
	// If it is a symlink, read its target path
	if (is_symlink) {
		ssize_t len = readlink(fname, n->sym_path, PATH_MAX - 1);
		if (len != -1) n->sym_path[len] = '\0'; // readlink does not auto null terminate
		else n->sym_path[0] = '\0'; // Handle readlink failure
	} else n->sym_path[0] = '\0';
	
	n->prev = *tail_ptr;
	
	// Append to the tail of the linked list
	// Use *head_ptr and *tail_ptr to access/modify the actual pointers in main()
	*tail_ptr = n;
}

// Free a linked list of subdirectories (SubDirNode).
void free_subfiles(SubDirFile *tail) {
    SubDirFile *cur = tail, *prev;
    while (cur != NULL) { // Stop when the true 'head' (prev is NULL) is reached
        prev = cur->prev; 
        free(cur);
        cur = prev;
    }
}

// ----------------- Handle file stats -----------------
// Updates the file counts and sizes for the current directory frame and the final report.
void HandleFiles(char *fname, DirFrame *frame, struct stat *st, struct stat *lst, 
				ActivityReport *report, bool show_files){
	if (S_ISREG(st->st_mode)) {  // Use stat() result to check if it's a regular file
		// update all of the local and global counts
		frame->dir_file_count++;
		frame->dir_file_size += st->st_size;
		report->TOTAL_file_count ++;	
		report->TOTAL_file_size += st->st_size;
		// Use lstat() result to check if the file entry itself is a symbolic link
		bool is_link = S_ISLNK(lst->st_mode);
		if (is_link) report->TOTAL_linked_files++;
		
		// if we are showing files, we need to push details onto the file stack
		if(show_files){
			char fdet[PATH_MAX] = "";
			char target[PATH_MAX] = "";
			
			if (is_link) {
				ssize_t len = readlink(fname, target, PATH_MAX - 1);
				if (len != -1) target[len] = '\0'; // readlink does not auto null terminate
				else target[0] = '\0'; // Handle readlink failure
			} else target[0] = '\0';

			snprintf(fdet, PATH_MAX, "%s%s%s%s%s", 
					is_link ? "@" : "", 
					strrchr(fname, '/') + 1,
					target[0] != '\0' ? " (" : "", 
					target[0] != '\0' ? target : "", 
					target[0] != '\0' ? ")" : "");
			add_subfile(is_link, fdet, &(frame->subfiles));
			// get file details
			// push onto file linked list
		}
	}
}

