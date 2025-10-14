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
    snprintf(out, outsz, (u == 0) ? "%.0f%s" : "%.1f%s", size, units[u]);
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
					  const char *entry_name, bool is_dir, bool colour_files) {
					   
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
        const char *TCOL  = "\033[36m";  // ANSI 33m cyan; 34 blue, 31 red, 32 green, 36 cyan
	    const char *RESET = "\033[0m";   // reset to default color

        printf("%s: " "%s%s%s\n",
        		depth == 0 ? "" : (is_last ? "    " : "│   "), 
        		colour_files ? TCOL : "", 
        		entry_name ? entry_name : "", 
        		colour_files ? RESET : "");
        return;
    }

    // --- DIRECTORY case: print connector + directory content (or symlink) ---
    if (depth > 0)
        printf("%s── ", is_last ? "└" : "├");

    // Use dir_name as the printed name for directories
    print_directory_content(dir_name, is_symdir, symPath, is_recursive, show_stats, fc, fs);
}

// ----------------- File Handling -------------------
// // Helper functions for maintaining a print_queue forfiles

static void add_subfile(bool is_symlink, char *fname, SubDirFile **tail_ptr){
	SubDirFile *n = xmalloc(sizeof(SubDirFile));
	snprintf(n->name, PATH_MAX, "%s", fname);
	n->is_symlink = is_symlink;
	n->sym_path[0] = '\0';
	n->prev = *tail_ptr;
	*tail_ptr = n;
}

// Free a linked list of subdirectories (SubDirFiles).
void free_subfiles(SubDirFile *tail) {
    SubDirFile *cur = tail, *prev;
    while (cur != NULL) { // Stop when the true 'head' (prev is NULL) is reached
        prev = cur->prev; 
        free(cur);
        cur = prev;
    }
}

// ----------------- Handle Files -----------------
// Handle files, symlinks, and dangling links properly
void HandleFiles(char *fname, DirFrame *frame, struct stat *st, struct stat *lst, 
                 ActivityReport *report, bool show_files) {

    bool target_is_file = false;
    bool target_is_dir = false;
    bool dangling = false;
    bool is_link;

    // Determine target type if symlink
    is_link = S_ISLNK(lst->st_mode);
    if (is_link) {
        if (st->st_mode == 0) {
            dangling = true; // stat failed
        } else if (S_ISDIR(st->st_mode)) {
            target_is_dir = true;
        } else if (S_ISREG(st->st_mode)) {
            target_is_file = true;
        }
    }

    // Case 1: regular file or symlink to file
    if ((!is_link && S_ISREG(st->st_mode)) || target_is_file) {
        frame->dir_file_count++;
        frame->dir_file_size += st->st_size;
        report->TOTAL_file_count++;
        report->TOTAL_file_size += st->st_size;
        if (is_link) report->TOTAL_linked_files++;

        if (show_files) {
            char fdet[PATH_MAX] = "";
            if (is_link) {
                char target[PATH_MAX] = "";
                ssize_t len = readlink(fname, target, PATH_MAX - 1);
                if (len != -1) target[len] = '\0';
                else target[0] = '\0';
                snprintf(fdet, PATH_MAX, "@%s (-> %s)", strrchr(fname, '/') + 1, target);
            } else {
                char hsize[32];
                human_size(st->st_size, hsize, sizeof(hsize));
                snprintf(fdet, PATH_MAX, "%s (%s)", strrchr(fname, '/') + 1, hsize);
            }
            add_subfile(is_link, fdet, &(frame->subfiles));
        }
        return;
    }

    // Case 2: dangling symlink (file or directory)
    if (is_link && dangling) {
        frame->dir_file_count++;
        report->TOTAL_file_count++;
        report->TOTAL_linked_files++;
        if (show_files) {
            char target[PATH_MAX] = "";
            ssize_t len = readlink(fname, target, PATH_MAX - 1);
            if (len != -1) target[len] = '\0';
            else target[0] = '\0';
            char fdet[PATH_MAX];
            snprintf(fdet, PATH_MAX, "@%s -> %s [dangling]", strrchr(fname, '/') + 1, target);
            add_subfile(true, fdet, &(frame->subfiles));
        }
        return;
    }

    // Case 3: symlink to directory (or directory itself)
    if (target_is_dir || (!is_link && S_ISDIR(st->st_mode))) {
        // Don't count as a file; traversal will handle directories
        return;
    }

    // Otherwise: ignore (non-regular, non-symlink files)
}



