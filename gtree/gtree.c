#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir (POSIX)
#include <string.h>     // For strncpy, strcmp, strrchr, memset, memcpy, snprintf
#include <sys/stat.h>   // For struct stat, lstat, stat, S_ISDIR, S_ISLNK (POSIX)
#include <libgen.h>     // For basename if needed (not used here)
#include <unistd.h>     // For readlink (POSIX)
#include <inttypes.h>   // For intmax_t

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

// -------------------- Loop Detection: visited directories linked list --------------------
// Stores inode/device ID pairs of all directories that have been successfully entered.
// Used to detect and avoid infinite loops when following symlinks.
typedef struct VisitedNode {
    dev_t st_dev;               // Device ID (unique per filesystem)
    ino_t st_ino;               // Inode number (unique per file on a filesystem)
    struct VisitedNode *next;   // Next node in visited list
} VisitedNode;

// -------------------------------- Final Report -------------------------------
// Holds summary stats accumulated during the traversal.
typedef struct ActivityReport {
	size_t TOTAL_file_count;           // Total number of regular files
	size_t TOTAL_linked_files;         // Number of regular files that are symbolic links
	off_t TOTAL_file_size;             // Total size of all regular files
	size_t TOTAL_directories;          // Total directories successfully traversed
	size_t TOTAL_linked_directories;   // Symlinked directories encountered
} ActivityReport;

// ------------------Memory safe allocation helpers (Wrapper Functions) ----------
// Wrappers around standard memory allocation functions (malloc/calloc/realloc)
// that perform error checking and exit the program on failure.
// This is a common pattern for robust C utilities.
void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL && size>0) {
        fprintf(stderr, "Fatal: Out of memory (malloc %zu bytes).\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (ptr == NULL && count>0 && size>0) {
        fprintf(stderr, "Fatal: Out of memory (calloc %zu count %zu bytes).\n", count, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL && size > 0) {
        fprintf(stderr, "Fatal: Out of memory (realloc %zu bytes).\n", size);
        free(ptr);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

// -----------------------------------------------------
// ------------------ Options Parsing ------------------
// -----------------------------------------------------

// Structure for command line option help definitions
typedef struct {
    const char *name;
    const char *help;
} HelpDef;

// Table for help messages for options
HelpDef help_table[] = {
    {"-h",   "Display this help message"},
	{"-s",   "Show File & Size totals for populated directories"},
	{"-l",   "Follow Sym-Link directories (disables loop-detection if not specified)"},
    {"-d N", "Maximum depth (will always run to a minimum of 2)"},
    {NULL, NULL} // sentinel: marks the end of the array
};

// List of supported options for getopt(). 'd:' means -d requires an argument.
const char option_list[] = "hsld:";

// Structure to hold all parsed command-line options
typedef struct {
    bool show_help;			// -h
    bool show_file_stats;	// -s
    bool follow_links;		// -l
    int max_depth;   		// -dN
} Options;

// Parses command line arguments using POSIX getopt() and sets the Options struct.
void parse_options(int argc, char *argv[], Options *opts, int *first_file_index) {
    *opts = (Options){0};           // Initialize all fields to 0 / false
    opts->max_depth = MAX_DEPTH;     // Default max depth
    int opt;
    // Loop through options using getopt. getopt returns -1 when no more options are found.
    while ((opt = getopt(argc, argv, option_list)) != -1) { 
        switch (opt) {
            case 'h': opts->show_help = true; break;
            case 's': opts->show_file_stats = true; break;
            case 'l': opts->follow_links = true; break;
            case 'd': {
                int n = atoi(optarg);        // optarg holds the argument for the current option (-d N)
                if (n < 2) n = 2;            // Enforce minimum depth
                if (n > MAX_DEPTH) n = MAX_DEPTH; // Prevent array overflow/extreme depth
                opts->max_depth = n;
                break;
			}
            default:
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    // After getopt finishes, optind is the index of the first non-option argument (the start path).
    if (optind < argc) *first_file_index = optind;
	else *first_file_index = -1;  
}

// Print help message using the help_table
void show_help(void){
	fprintf(stderr, "Usage: fs [options] starting_directory \n");
	fprintf(stderr, "Options:\n");
	for (HelpDef *opt = help_table; opt->name; opt++) {
		fprintf(stderr, "  %s\t%s\n", opt->name, opt->help);
	}
	fprintf(stderr, "Version %s\n", TH_VERSION);
}

// ------------------- Visited linked list helpers ------------------
// Used to prevent infinite loops when following symlinks
VisitedNode* add_visited(VisitedNode **head, dev_t dev, ino_t ino) {
    // Allocate new node using the safe allocation wrapper
    VisitedNode *n = xmalloc(sizeof(VisitedNode));
    n->st_dev = dev;
    n->st_ino = ino;
    // Prepend to the front of the list for O(1) insertion
    n->next = *head;
    *head = n;
    return n;
}

// Checks if a directory (identified by its unique dev/ino pair) has been visited before.
bool visited_before(VisitedNode *head, dev_t dev, ino_t ino) {
    for (VisitedNode *cur = head; cur; cur = cur->next)
        if (cur->st_dev == dev && cur->st_ino == ino)
            return true; // Match found
    return false;
}

// Frees all memory used by the visited directories linked list.
void free_visited(VisitedNode *head) {
    VisitedNode *cur = head, *next;
    while (cur) { next = cur->next; free(cur); cur = next; }
}

// Free a linked list of subdirectories (SubDirNode).
void free_subdirs(SubDirNode *head) {
    SubDirNode *cur = head, *next;
    while (cur) { 
        next = cur->next; 
        free(cur);
        cur = next;
    }
}

// ----------------- Human readable file size -------------------
// Converts a size in bytes (off_t) to a human-readable string (e.g., 4.5K, 2.1M).
static void human_size(off_t bytes, char *out, size_t outsz)
{
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

// ----------------- Printing a directory line -------------------
// Core function for printing a line, handling tree symbols and stats.
void print_directory_line(const char* basePath, int depth, bool is_last,
                          bool has_sibling[], bool is_symdir, const char* symPath, 
                          size_t fc, off_t fs, bool is_recursive, bool show_stats)
{
    const char* name;
    // Extract just the basename of the directory for clean printing
    const char *slash = strrchr(basePath, '/');
    name = (slash && depth > 0) ? slash + 1 : basePath;  

    // Print tree structure prefix (e.g., │    )
    for (int i = 1; i < depth; i++)
        // Print vertical bar '│' if the ancestor has more siblings, else print spaces
        printf("%s", has_sibling[i] ? "│   " : "    ");

    // Print connector for the current level (└ or ├)
    if (depth > 0)
        printf("%s── ", is_last ? "└" : "├");

    // Case: Directory is a symbolic link
    if (is_symdir) {
        printf("@%s -> %s%s\n", name, symPath, is_recursive ? " [recursive - not followed]" : "");
        return;
    }

    // Case: Normal directory (print file stats if any)
    if(show_stats && fc>0){
		char hsize[32];
		human_size(fs, hsize, sizeof(hsize));
    	printf("%s [Files: %zu] [Size: %s]%s\n", name, fc, hsize, is_recursive ? " [recursive - not followed]" : "");
    } 
    else {
    	printf("%s%s\n", name, is_recursive ? " [recursive - not followed]" : "");
	}
}

// ----------------- Create a new directory frame -----------------
// Allocates and initializes a new DirFrame, simulating a push onto the call stack.
DirFrame *Create_Frame(const char *dirPath, int dirDepth, const DirFrame *parent, bool is_last){
    DirFrame *dirptr = xmalloc(sizeof(DirFrame));
    snprintf(dirptr->path, PATH_MAX, "%s", dirPath);
    dirptr->depth = dirDepth;
    dirptr->is_last = is_last;
    
    // Copy has_sibling state from parent to maintain correct tree formatting
    if (parent)
        memcpy(dirptr->has_sibling, parent->has_sibling, sizeof(parent->has_sibling));
    else // Root node initialization
        memset(dirptr->has_sibling, 0, sizeof(dirptr->has_sibling));
    
    // Attempt to open the directory for reading its entries (Phase 1 preparation)
    dirptr->dir = opendir(dirPath);    
    if (!dirptr->dir) { perror("opendir"); free(dirptr); return NULL; }
    
    // Initialize for Phase 1 (scanning)
    dirptr->subdirs = NULL;
    dirptr->current = NULL;
    dirptr->dir_file_count = 0;
    dirptr->dir_file_size = 0;
    return dirptr;
}

// ----------------- Handle file stats -----------------
// Updates the file counts and sizes for the current directory frame and the final report.
void HandleFiles(DirFrame *frame, struct stat *st, struct stat *lst, ActivityReport *report){
	if (S_ISREG(st->st_mode)) {  // Use stat() result to check if it's a regular file
		frame->dir_file_count++;
		frame->dir_file_size += st->st_size;
		report->TOTAL_file_count ++;
		
		// Use lstat() result to check if the file entry itself is a symbolic link
		if (S_ISLNK(lst->st_mode)) report->TOTAL_linked_files++;
		
		report->TOTAL_file_size += st->st_size;
	}
}

// ------------------------- Main -------------------------
int main(int argc, char *argv[]) {
    Options opts;
    int first_file_index;	
    // Parse command line options first
    parse_options(argc, argv, &opts, &first_file_index);

	if (opts.show_help || first_file_index == -1){
		show_help();
		return EXIT_SUCCESS;
	}

	ActivityReport final_report = {0};    // Initialize all counters to 0
    // The explicit stack for DirFrame pointers.
    DirFrame *stack[MAX_DEPTH+2];         
    int sp = 0;                           // Stack pointer (index of the next free slot)

    VisitedNode *visited_root = NULL;     // Head of visited directories list (for loop detection)

    // Create and initialize the root frame
    DirFrame *root = Create_Frame(argv[first_file_index], 0, NULL, false);

    // Get stat info for the starting directory to mark it as visited
    struct stat st_root;
    if (stat(root->path, &st_root) == 0)
        // Record the root directory's unique ID (dev/ino) to prevent re-entry via symlink
        add_visited(&visited_root, st_root.st_dev, st_root.st_ino); 

    stack[sp++] = root;  // Push root onto the explicit stack

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

            HandleFiles(frame, &st, &lst, &final_report);

            // Determine if the entry is a symlink *that points to* a directory
            bool is_symdir = false;
            if (S_ISLNK(lst.st_mode) && S_ISDIR(st.st_mode))
                is_symdir = true;

            // Add to subdirectory linked list if it's a normal directory OR a symlinked directory
            if (S_ISDIR(st.st_mode) || is_symdir) {
                SubDirNode *n = xmalloc(sizeof(SubDirNode));
                snprintf(n->path, PATH_MAX, "%s", buf);
                n->is_symlink = is_symdir;
                
                // If it is a symlink, read its target path
                if (is_symdir) {
                    ssize_t len = readlink(buf, n->sym_path, PATH_MAX - 1);
                    if (len != -1) n->sym_path[len] = '\0';
                    else n->sym_path[0] = '\0'; // Handle readlink failure
                } else n->sym_path[0] = '\0';
                
                n->next = NULL;
                // Append to the tail of the linked list
                if (!head) head = n;
                else tail->next = n;
                tail = n;
            }
        }

        // Mark Phase 1 complete and set up for Phase 2 (descending)
        frame->subdirs = head;
        frame->current = head;

        // Print the current directory line (must happen *after* file scanning)
        print_directory_line(frame->path, frame->depth, frame->is_last,
                             frame->has_sibling, false, NULL,
                             frame->dir_file_count, frame->dir_file_size, false, opts.show_file_stats);
    }

    // ----------------- Phase 2: Process the next subdirectory -----------------
    if (frame->current) {
        SubDirNode *cur = frame->current;
        frame->current = cur->next;        // Advance linked list iterator for the next loop iteration
        bool is_last_child = (frame->current == NULL);
        
        // Update the has_sibling array for the *next* depth level
        if (frame->depth + 1 < opts.max_depth)
            frame->has_sibling[frame->depth + 1] = !is_last_child;

        // Get the unique ID (dev/ino) of the target directory
        struct stat st_target;
        bool stat_ok = (stat(cur->path, &st_target) == 0); // stat() follows the link

        // Handle symbolic directories
		if (cur->is_symlink) {
			final_report.TOTAL_linked_directories++;
			final_report.TOTAL_directories++; // it's still a directory, even though it's sym linked
			// Check if the symlink target (dev/ino) has already been visited
			bool already_visited = stat_ok && visited_before(visited_root, st_target.st_dev, st_target.st_ino);
		
			// Determine if the symlink printout should use the '└' (last) branch symbol
			bool show_as_last = is_last_child && !(opts.follow_links && stat_ok && !already_visited);
		
			// Print the symlink line
			print_directory_line(cur->path, frame->depth + 1, show_as_last,
								 frame->has_sibling, true, cur->sym_path,
								 0, 0, already_visited, opts.show_file_stats);
		
			// Follow link if allowed by options and not already visited
			if (!already_visited && opts.follow_links && stat_ok) {
				// Mark the target as visited *before* pushing, to protect from internal loops
				add_visited(&visited_root, st_target.st_dev, st_target.st_ino);
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
            add_visited(&visited_root, st_target.st_dev, st_target.st_ino);

			// Create new frame and push onto the stack
            DirFrame *child = Create_Frame(cur->path, frame->depth + 1, frame, is_last_child);
            if (child) stack[sp++] = child;
        }

    } else {
        // Directory fully processed (both files scanned and all subdirs handled).
        // Pop and clean up: This simulates the function returning in recursion (backtracking).
        closedir(frame->dir);
        free_subdirs(frame->subdirs);
        free(frame);
        sp--;
    }
}

    free_visited(visited_root); // Clean up memory for the loop-detection list

    // Print summary report
    char hsize[32];
    human_size(final_report.TOTAL_file_size, hsize, sizeof(hsize));
    printf( "Total Number of Files: %zu (of which %zu are linked)\n"
    		"Total File Size: %s\n"
    		"Total Number of Directories traversed %zu (of which %zu are linked)\n", 
    		final_report.TOTAL_file_count, final_report.TOTAL_linked_files, hsize, 
    		final_report.TOTAL_directories, final_report.TOTAL_linked_directories);

    return 0;
}