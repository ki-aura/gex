/*
================================================================================
gtree Traversal Logic (Explicit Stack Version)
================================================================================
This diagram explains how the gtree program simulates recursive directory 
traversal using an explicit stack of DirFrame pointers

Legend:
  [DirFrame]   : Represents a directory currently being traversed.
  stack[sp]    : Explicit stack holding DirFrame pointers.
  subdirs      : Linked list of subdirectories inside current directory.
  current      : Pointer to the next subdirectory to process in subdirs.
  sp           : Stack pointer; points to the next free slot in the stack.
  is_symlink   : Indicates whether a directory entry is a symlink.
  ancestor_siblings[] : Array used for drawing tree structure correctly.

================================================================================
Notes for Understanding:
- The explicit stack simulates the call stack used in a recursive implementation.
- Phase 1 = "Scan": build the list of subdirectories to traverse.
- Phase 2 = "Process": traverse subdirectories using the stack.
- This approach avoids actual recursion, giving better control over stack size.
- Symlink handling and visited hash prevent infinite loops caused by recursive links.
- ancestor_siblings[] ensures proper tree drawing even with deep nested directories.

================================================================================
High-level Algorithm:

1. Initialize
   - Parse command line options.
   - Create a hash table for visited directories to avoid infinite loops via symlinks.
   - Create root DirFrame for starting directory.
   - Push root onto stack.
   - Initialize ActivityReport counters.

2. Main Traversal Loop (while sp > 0)
   a) Peek at top frame on stack:
        DirFrame *frame = stack[sp - 1];

- We are "peeking" at the top of the stack to see which directory we should process 
	next without removing it yet.
- The stack contains DirFrame* pointers, with the next free slot always indicated by sp.
- sp is the index of the next available slot, not the current top element. Therefore, 
	the current top frame is at sp - 1.
- Peeking allows us to examine the current directory (its entries, subdirectories, and 
	files) while still keeping it on the stack. We only pop it after all its subdirectories 
	have been fully processed.
- This is essential for simulating recursion explicitly: in a normal recursive call, the 
	function’s local variables remain on the call stack until the function returns. Here, 
	the stack array and DirFrame pointers play the same role.
        

   b) Phase 1: Scan Current Directory (only if subdirs == NULL)
        - Read entries with readdir().
        - Skip "." and "..".
        - Build full path for each entry.
        - lstat() for symlink info, stat() for actual file type.
        - Handle files (update file count/size, print if requested).
        - For directories or symlinked directories:
            * Check if already visited (loop prevention).
            * If not visited, create SubDirNode and append to subdirs list.
        - Set frame->subdirs = head of SubDirNode list.
        - Set frame->current = head.
        - Print current directory line.
        - Print files if requested.

   c) Phase 2: Process Next Subdirectory
        - If frame->current != NULL:
            * Take current SubDirNode.
            * Advance frame->current to next node.
            * Determine if this is the last child.
            * Update ancestor_siblings[] for correct tree drawing.
            * Perform stat() to get directory info.
            * If symlink:
                - Print entry line with target path.
                - Traverse if not already visited and follow_links enabled:
                    + Create new DirFrame for child directory.
                    + Push onto stack.
                    + Increment TOTAL_directories.
                    + Track max depth.
                - Increment TOTAL_linked_directories.
            * If normal directory and not visited:
                - Create DirFrame.
                - Push onto stack.
                - Increment TOTAL_directories.
                - Track max depth.
        - Else (frame->current == NULL):
            * Directory fully processed.
            * Pop frame from stack.
            * Close directory and free memory (subdirs list + frame).

3. Loop ends when stack is empty.
4. Cleanup:
    - Free visited node hash table.
5. Print summary statistics:
    - Total directories, linked directories, files, total file size, max depth.

================================================================================
Example Stack Visualization (simplified):
================================================================================

Initial:
sp = 0
stack = []

Push root:
sp = 1
stack = [ root ]

Phase 1: scan root
  stack[0].subdirs = [subdir1, subdir2]
  stack[0].current = subdir1

Phase 2: process subdir1
  create child frame for subdir1
  sp = 2
  stack = [root, subdir1]

Pop subdir1 after processing all its children:
  sp = 1
  stack = [root]

Continue with subdir2...
================================================================================
*/


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

// ----------------- Helper function -----------------
// Update the maximum depth reached during traversal
static inline void track_max_depth(ActivityReport *report, int current_depth) {
    if (report->TOTAL_depth < current_depth) {
        report->TOTAL_depth = current_depth;
    }
}

// ----------------- Create a new directory frame -----------------
// Allocates and initializes a new DirFrame, simulating a push onto an explicit stack
DirFrame *Create_Frame(const char *dirPath, int dirDepth, const DirFrame *parent, bool is_last) {
    DirFrame *framePtr = xmalloc(sizeof(DirFrame));

    // Copy directory path into frame
    snprintf(framePtr->path, PATH_MAX, "%s", dirPath);
    framePtr->depth = dirDepth;
    framePtr->is_last = is_last;

    // Copy ancestor_siblings from parent for correct tree formatting
    if (parent)
        memcpy(framePtr->ancestor_siblings, parent->ancestor_siblings, sizeof(parent->ancestor_siblings));
    else // Root node initialization
        memset(framePtr->ancestor_siblings, 0, sizeof(framePtr->ancestor_siblings));

    // Attempt to open the directory for reading entries
    framePtr->dir = opendir(dirPath);
    if (!framePtr->dir) { 
        perror("opendir"); 
        free(framePtr); 
        return NULL; 
    }

    // Initialize Phase 1 (scanning) variables
    framePtr->subdirs = NULL;
    framePtr->current = NULL;
    framePtr->subfiles = NULL;
    framePtr->dir_file_count = 0;
    framePtr->dir_file_size = 0;

    return framePtr;
}

// ----------------- Add a subdirectory node -----------------
// Appends a new SubDirNode to the linked list, updating head and tail pointers
void add_subdir(bool is_symdir, char *sub_path, SubDirNode **head_ptr, SubDirNode **tail_ptr) {
    // Allocate a new node and populate its path
    SubDirNode *n = xmalloc(sizeof(SubDirNode));
    snprintf(n->path, PATH_MAX, "%s", sub_path);
    n->is_symlink = is_symdir;

    // If it's a symlink, read its target path
    if (is_symdir) {
        ssize_t len = readlink(sub_path, n->sym_path, PATH_MAX - 1);
        if (len != -1) n->sym_path[len] = '\0'; // readlink does not null-terminate
        else n->sym_path[0] = '\0'; // readlink failed
    } else {
        n->sym_path[0] = '\0';
    }

    n->next = NULL;

    // Append node to linked list
    if (!*head_ptr) {
        *head_ptr = n;
        *tail_ptr = n; // first node: tail points here too
    } else {
        (*tail_ptr)->next = n;
        *tail_ptr = n;
    }
}

// ----------------- Free linked list of subdirectories -----------------
void free_subdirs(SubDirNode *head) {
    SubDirNode *cur = head, *next;
    while (cur) { 
        next = cur->next;
        free(cur);
        cur = next;
    }
}

// ------------------------- Main function -------------------------
int main(int argc, char *argv[]) {
    Options opts;
    int first_file_index;

    // Parse command line options
    parse_options(argc, argv, &opts, MAX_DEPTH, &first_file_index);

    // If help requested or invalid file index, show help and exit
    if (opts.show_help || first_file_index == -1 || first_file_index < argc - 1) {
        printf("%d %d\n", first_file_index, argc);
        show_help();
        return EXIT_SUCCESS;
    }

    // Initialize all counters to 0
    ActivityReport final_report = {0};

    // Explicit stack for DirFrame pointers (simulate recursion)
    DirFrame *stack[MAX_DEPTH + 2];
    int sp = 0; // stack pointer: next free slot

    // Hash table to track visited directories to prevent infinite recursion via symlinks
    create_visited_node_hash();

    // Create root frame
    DirFrame *root = Create_Frame(argv[first_file_index], 0, NULL, false);

    // Record root directory's unique device/inode ID
    struct stat st_root;
	if (stat(root->path, &st_root) == 0) {
		if (add_visited(st_root.st_dev, st_root.st_ino))
			final_report.TOTAL_directories++;
	}

    // Push root frame onto stack
    stack[sp++] = root;
	track_max_depth(&final_report, root->depth);

    // ------------------ Main traversal loop ------------------
    // Loop continues while there are frames (directories) on the stack
    while (sp > 0) {
        DirFrame *frame = stack[sp - 1]; // peek at top of stack

        // ------------------ Phase 1: Scan current directory ------------------
        if (!frame->subdirs) {
            struct dirent *entry;
            struct stat st, lst;
            char buf[PATH_MAX];
            SubDirNode *head = NULL, *tail = NULL;

            frame->dir_file_count = 0;
            frame->dir_file_size = 0;

            // Read each entry in the directory
            while ((entry = readdir(frame->dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                    continue;

                // Build full path
                if (snprintf(buf, PATH_MAX, "%s/%s", frame->path, entry->d_name) >= PATH_MAX)
                    continue;

                if (lstat(buf, &lst) == -1) continue;
                if (stat(buf, &st) == -1) st.st_mode = 0;

                // Handle files (update stats, print if needed)
                HandleFiles(buf, frame, &st, &lst, &final_report, opts.show_files);

                bool is_symdir = S_ISLNK(lst.st_mode) && S_ISDIR(st.st_mode);

                // Check if directory already visited
                bool already_visited = false;
                if ((S_ISDIR(st.st_mode) || is_symdir) && stat(buf, &st) == 0)
                    already_visited = visited_before(st.st_dev, st.st_ino);

                // Add subdirectory to list (regardless of if visited - this is checked in phase 2)
                if (S_ISDIR(st.st_mode) || is_symdir)
                    add_subdir(is_symdir, buf, &head, &tail);
            }

            // Save list to frame
            frame->subdirs = head;
            frame->current = head;

            // Print current directory line
            print_entry_line(frame, frame->is_last,
                             false, NULL,
                             false, opts.show_file_stats, NULL, true);

            // Print files if requested
            if (opts.show_files) {
                SubDirFile *cur = frame->subfiles, *prev;
                while (cur != NULL) {
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
            frame->current = cur->next;        // advance iterator
            bool is_last_child = (frame->current == NULL);

            // Update ancestor_siblings array for next depth
            if (frame->depth + 1 < opts.max_depth)
                frame->ancestor_siblings[frame->depth + 1] = !is_last_child;

            struct stat st_target;
            bool stat_ok = (stat(cur->path, &st_target) == 0); // follow symlink

            // ---------------- Symlinked directories ----------------
            if (cur->is_symlink) {
                bool already_visited = stat_ok && visited_before(st_target.st_dev, st_target.st_ino);

                // Prepare temporary frame for printing
                DirFrame temp = {0};
                snprintf(temp.path, PATH_MAX, "%s", cur->path);
                temp.depth = frame->depth + 1;
                memcpy(temp.ancestor_siblings, frame->ancestor_siblings, sizeof(temp.ancestor_siblings));

                print_entry_line(&temp, is_last_child, true, cur->sym_path,
                                 already_visited, opts.show_file_stats, NULL, true);

                // Traverse if not already visited and option allows
                if (!already_visited && opts.follow_links && stat_ok) {
                    DirFrame *child = Create_Frame(cur->path, frame->depth + 1, frame, is_last_child);
                    if (child) {
						if (add_visited(st_target.st_dev, st_target.st_ino)) {
							final_report.TOTAL_directories++;
						}
                        stack[sp++] = child;
                        track_max_depth(&final_report, child->depth);
                    }
                }

                // increment total linked directories even if not traversed
                if (stat_ok) final_report.TOTAL_linked_directories++;
                continue; // move to next subdirectory
            }

            // ---------------- Normal directories ----------------
            if (stat_ok && S_ISDIR(st_target.st_mode)) {
                // skip if already visited
                if (!visited_before(st_target.st_dev, st_target.st_ino) && sp < opts.max_depth) {
                    DirFrame *child = Create_Frame(cur->path, frame->depth + 1, frame, is_last_child);
                    if (child) {
						if (add_visited(st_target.st_dev, st_target.st_ino)) {
							final_report.TOTAL_directories++;
						}
                        stack[sp++] = child;
                        track_max_depth(&final_report, child->depth);
                    }
                } else {
                	// print as RECURSIVE
					DirFrame temp = {0};
					snprintf(temp.path, PATH_MAX, "%s", cur->path);
					temp.depth = frame->depth + 1;
					memcpy(temp.ancestor_siblings, frame->ancestor_siblings, sizeof(temp.ancestor_siblings));
	
					print_entry_line(&temp, is_last_child, false, cur->path,
									 true, opts.show_file_stats, NULL, true);
                }
            }

        } else {
            // Directory fully processed: pop and clean up
            closedir(frame->dir);
            free_subdirs(frame->subdirs);
            free(frame);
            sp--;
        }
    }

    // ----------------- Clean up -----------------
    free_visited_node_hash(); // free memory for loop-detection hash

    // ----------------- Print summary -----------------
    char hsize[32];
    human_size(final_report.TOTAL_file_size, hsize, sizeof(hsize));
    printf("\nTotal Number of Directories traversed %zu (of which %zu are linked)\n"
           "Maximum depth descended: %d\n", 
           final_report.TOTAL_directories, final_report.TOTAL_linked_directories, final_report.TOTAL_depth);

    if (opts.show_file_stats || opts.show_files)
        printf("Total Number of Files: %zu (of which %zu are linked)\n"
               "Total File Size: %s\n",
               final_report.TOTAL_file_count, final_report.TOTAL_linked_files, hsize);

    return 0;
}
