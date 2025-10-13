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
#include "visit_hash.h"
#include "option_parsing.h"
#include "memsafe.h"


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
void print_entry_line(const DirFrame *frame,
                      bool is_last,
                      bool is_symdir,
                      const char *symPath,
                      bool is_recursive,
                      bool show_stats,
                      const char *entry_name,
                      bool is_dir)
{
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



// ----------------- Create a new directory frame -----------------
// Allocates and initializes a new DirFrame, simulating a push onto the call stack.
DirFrame *Create_Frame(const char *dirPath, int dirDepth, const DirFrame *parent, bool is_last){
    DirFrame *framePtr = xmalloc(sizeof(DirFrame));
    snprintf(framePtr->path, PATH_MAX, "%s", dirPath);
    framePtr->depth = dirDepth;
    framePtr->is_last = is_last;
    
    // Copy ancestor_siblings state from parent to maintain correct tree formatting
    if (parent)
        memcpy(framePtr->ancestor_siblings, parent->ancestor_siblings, sizeof(parent->ancestor_siblings));
    else // Root node initialization
        memset(framePtr->ancestor_siblings, 0, sizeof(framePtr->ancestor_siblings));
    
    // Attempt to open the directory for reading its entries (Phase 1 preparation)
    framePtr->dir = opendir(dirPath);    
    if (!framePtr->dir) { perror("opendir"); free(framePtr); return NULL; }
    
    // Initialize for Phase 1 (scanning)
    framePtr->subdirs = NULL;
    framePtr->current = NULL;
    framePtr->subfiles = NULL;
    framePtr->dir_file_count = 0;
    framePtr->dir_file_size = 0;
    return framePtr;
}

// ----------------- Create a new subdirectory node and add to list -----------------
// Modified to take pointers to the head and tail pointers (SubDirNode **)
void add_subdir(bool is_symdir, char *sub_path, SubDirNode **head_ptr, SubDirNode **tail_ptr){
	// allocate a new node & populate it
	SubDirNode *n = xmalloc(sizeof(SubDirNode));
	snprintf(n->path, PATH_MAX, "%s", sub_path);
	n->is_symlink = is_symdir;
	
	// If it is a symlink, read its target path
	if (is_symdir) {
		ssize_t len = readlink(sub_path, n->sym_path, PATH_MAX - 1);
		if (len != -1) n->sym_path[len] = '\0'; // readlink does not auto null terminate
		else n->sym_path[0] = '\0'; // Handle readlink failure
	} else n->sym_path[0] = '\0';
	
	n->next = NULL;
	
	// Append to the tail of the linked list
	// Use *head_ptr and *tail_ptr to access/modify the actual pointers in main()
	if (!*head_ptr) {
        *head_ptr = n;
        *tail_ptr = n; // If it's the first node, tail must also point to it
    } else {
        (*tail_ptr)->next = n;
        *tail_ptr = n;
    }
}

// ----------------- Free a linked list of subdirectories (SubDirNode) -----------------
void free_subdirs(SubDirNode *head) {
    SubDirNode *cur = head, *next;
    while (cur) { 
        next = cur->next; 
        free(cur);
        cur = next;
    }
}


void add_subfile(bool is_symlink, char *fname, SubDirFile **tail_ptr){
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
void HandleFiles(char *fname, DirFrame *frame, struct stat *st, struct stat *lst, ActivityReport *report, bool show_files){
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

// ------------------------- Main -------------------------
int main(int argc, char *argv[]) {
    Options opts;
    int first_file_index;	
    // Parse command line options first
    parse_options(argc, argv, &opts, MAX_DEPTH, &first_file_index);

	if (opts.show_help || first_file_index == -1){
		show_help();
		return EXIT_SUCCESS;
	}

	ActivityReport final_report = {0};    // Initialize all counters to 0
    // The explicit stack for DirFrame pointers.
    DirFrame *stack[MAX_DEPTH+2];         
    int sp = 0;                           // Stack pointer (index of the next free slot)

	// this hash table will store the node and device id's of every directory visited 
	// required to avoid multiple travels down sym_linked directories that form a recursive loop
	create_visited_node_hash();
	
    // Create and initialize the root frame
    DirFrame *root = Create_Frame(argv[first_file_index], 0, NULL, false);

    // Get stat info for the starting directory to mark it as visited
    struct stat st_root;
    if (stat(root->path, &st_root) == 0)
        // Record the root directory's unique ID (dev/ino) to prevent re-entry via symlink
        add_visited(st_root.st_dev, st_root.st_ino); 

    stack[sp++] = root;  // Push root onto the explicit stack
    // NOTE sp always points to the next available frame, not the current one!

	// ------------------ Main traversal loop ------------------
	// Loop continues as long as there are frames (directories) on the stack.
	while (sp > 0) {
		DirFrame *frame = stack[sp - 1]; // Peek: Get the top frame without popping
	
		// Phase 1: Scan the current directory for files and subdirectories.
		if (!frame->subdirs) {
			struct dirent *entry;
			struct stat st, lst;  // st for file status, lst for link status (lstat)
			char buf[PATH_MAX];    // Buffer for constructing the full path
			SubDirNode *head = NULL, *tail = NULL;
	
			frame->dir_file_count = 0;
			frame->dir_file_size = 0;
	
			// Scan directory entries using readdir()
			while ((entry = readdir(frame->dir)) != NULL) {
				// Skip current ('.') and parent ('..') directory entries
				if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
					continue;
	
				// Construct the full path (ParentPath/EntryName)
				if (snprintf(buf, PATH_MAX, "%s/%s", frame->path, entry->d_name) >= PATH_MAX)
					continue;
	
				// lstat gets info about the link itself (if it is one)
				if (lstat(buf, &lst) == -1) continue;
				// stat follows the link to get info about the target (if it exists)
				if (stat(buf, &st) == -1) st.st_mode = 0;
	
				HandleFiles(buf, frame, &st, &lst, &final_report, opts.show_files);
	
				// Determine if the entry is a symlink *that points to* a directory
				bool is_symdir = false;
				if (S_ISLNK(lst.st_mode) && S_ISDIR(st.st_mode))
					is_symdir = true;
	
				// Add to subdirectory linked list if it's a normal directory OR a symlinked directory
				if (S_ISDIR(st.st_mode) || is_symdir) {
					add_subdir(is_symdir, buf, &head, &tail);
				}
			}
	
			// Mark Phase 1 complete and set up for Phase 2 (descending)
			frame->subdirs = head;
			frame->current = head;
	
			// Print the current directory line (must happen *after* file scanning)
			print_entry_line(frame, frame->is_last,
                 false, NULL,
                 false, opts.show_file_stats, NULL, true);
                 
			if(opts.show_files){
				SubDirFile *cur = frame->subfiles, *prev;
				while (cur != NULL) { // Stop when the true 'head' (prev is NULL) is reached
					prev = cur->prev; 
					print_entry_line(frame, frame->is_last,
               			  cur->is_symlink, NULL,
                		  false, opts.show_file_stats, cur->name, false);
					cur = prev;
				}
				free_subfiles(frame->subfiles);
			}								 
		}
	
		// ----------------- Phase 2: Process the next subdirectory -----------------
		if (frame->current) {
			SubDirNode *cur = frame->current;
			frame->current = cur->next;        // Advance linked list iterator for the next loop iteration
			bool is_last_child = (frame->current == NULL);
			
			// Update the ancestor_siblings array for the *next* depth level
			if (frame->depth + 1 < opts.max_depth)
				frame->ancestor_siblings[frame->depth + 1] = !is_last_child;
	
			// Get the unique ID (dev/ino) of the target directory
			struct stat st_target;
			bool stat_ok = (stat(cur->path, &st_target) == 0); // stat() follows the link
	
			// Handle symbolic directories
			if (cur->is_symlink) {
				final_report.TOTAL_linked_directories++;
				final_report.TOTAL_directories++; // it's still a directory, even though it's sym linked
				// Check if the symlink target (dev/ino) has already been visited
				bool already_visited = stat_ok && visited_before(st_target.st_dev, st_target.st_ino);
			
				// Determine if the symlink printout should use the '└' (last) branch symbol
				// logic - it should if it's the last child, unless we're following a valid symlink down
				bool show_as_last = is_last_child && !(opts.follow_links && stat_ok && !already_visited);
			
				// Print the symlink line
				DirFrame temp = {0};
				snprintf(temp.path, PATH_MAX, "%s", cur->path);
				temp.depth = frame->depth + 1;
				memcpy(temp.ancestor_siblings, frame->ancestor_siblings, sizeof(temp.ancestor_siblings));
				
				print_entry_line(&temp, show_as_last,
							 true, cur->sym_path,
							 already_visited, opts.show_file_stats, NULL, true);
			
				// Follow link if allowed by options and not already visited
				if (!already_visited && opts.follow_links && stat_ok) {
					// Mark the target as visited *before* pushing, to protect from internal loops
					add_visited(st_target.st_dev, st_target.st_ino);
					if (sp < opts.max_depth) {
						// Create new frame for the target directory and push onto stack
						DirFrame *child = Create_Frame(cur->path, frame->depth + 1, frame, is_last_child);
						if (child) stack[sp++] = child; 
					}
				}
				continue; // Move to the next subdirectory in the current frame
			}
	
			// Normal directories
			if (stat_ok && S_ISDIR(st_target.st_mode)) {
				// Check against the user-defined maximum depth
				if (sp >= opts.max_depth) continue;
	
				final_report.TOTAL_directories++;
				// Mark normal directories as visited (by their target dev/ino)
				add_visited(st_target.st_dev, st_target.st_ino);
	
				// Create new frame and push onto the stack
				DirFrame *child = Create_Frame(cur->path, frame->depth + 1, frame, is_last_child);
				if (child) stack[sp++] = child;
			}
	
		} else {
			// Directory fully processed (files scanned and all subdirs handled).
			// Pop and clean up: This simulates the function returning in recursion (backtracking).
			closedir(frame->dir);
			free_subdirs(frame->subdirs);
			free(frame);
			sp--;
		}
	}

    //free_visited(visited_root); // Clean up memory for the loop-detection list
    free_visited_node_hash(); // Clean up memory for the loop-detection hash

    // Print summary report
    char hsize[32];
    human_size(final_report.TOTAL_file_size, hsize, sizeof(hsize));
    printf( "\nTotal Number of Directories traversed %zu (of which %zu are linked)\n", 
    		final_report.TOTAL_directories, final_report.TOTAL_linked_directories);
 
    if(opts.show_file_stats)
		printf( "Total Number of Files: %zu (of which %zu are linked)\n"
				"Total File Size: %s\n",
				final_report.TOTAL_file_count, final_report.TOTAL_linked_files, hsize);
	 
    return 0;
}