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
#include "print.h"

// helper function for tracking max depth
static inline void track_max_depth(ActivityReport *report, int current_depth) {
    if (report->TOTAL_depth < current_depth) {
        report->TOTAL_depth = current_depth;
    }
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



// ------------------------- Main -------------------------
int main(int argc, char *argv[]) {
    Options opts;
    int first_file_index;	
    // Parse command line options first
    parse_options(argc, argv, &opts, MAX_DEPTH, &first_file_index);

	if (opts.show_help || first_file_index == -1 || first_file_index < argc -1){
		printf("%d %d\n", first_file_index, argc);
		show_help();
		return EXIT_SUCCESS;
	}

	ActivityReport final_report = {0};    // Initialize all counters to 0
    // The explicit stack for DirFrame pointers.
    DirFrame *stack[MAX_DEPTH+2];         
    int sp = 0;            // Stack pointer (index of the next free slot)

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

    stack[sp++] = root; track_max_depth(&final_report, sp); // Push root onto the explicit stack
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
						if (child) {stack[sp++] = child; track_max_depth(&final_report, sp);}
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
				if (child) {stack[sp++] = child; track_max_depth(&final_report, sp);}
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
    printf( "\nTotal Number of Directories traversed %zu (of which %zu are linked)\n"
    		"Maximum depth descended: %d\n", 
    		final_report.TOTAL_directories, final_report.TOTAL_linked_directories, final_report.TOTAL_depth);
 
    if(opts.show_file_stats || opts.show_files)
		printf( "Total Number of Files: %zu (of which %zu are linked)\n"
				"Total File Size: %s\n",
				final_report.TOTAL_file_count, final_report.TOTAL_linked_files, hsize);
	 
    return 0;
}