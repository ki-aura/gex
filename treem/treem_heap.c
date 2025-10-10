#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir
#include <string.h>     // For strncpy, strcmp, strrchr, memset, memcpy, snprintf
#include <sys/stat.h>   // For struct stat, lstat, stat, S_ISDIR, S_ISLNK
#include <libgen.h>     // For basename if needed (not used here)
#include <unistd.h>     // For readlink
#include <inttypes.h>   // For intmax_t

#ifndef PATH_MAX
#define PATH_MAX 1024   // Maximum path length if not defined
#endif

#define MAX_DEPTH 512   // Maximum depth for directory traversal / stack size

// Node structure to hold subdirectory info
typedef struct SubDirNode {
    char path[PATH_MAX];       // Full path of the subdirectory
    bool is_symlink;           // True if this directory is a symlink
    char sym_path[PATH_MAX];   // Target path if symlink
    struct SubDirNode *next;   // Next subdirectory in the linked list
} SubDirNode;

// Frame structure representing one directory level in the stack
typedef struct DirFrame {
    char path[PATH_MAX];         // Path of this directory
    DIR *dir;                    // DIR* for reading entries
    SubDirNode *subdirs;         // Linked list of subdirectories
    SubDirNode *current;         // Pointer to current subdirectory being processed
    int depth;                   // Depth in the tree (0=root)
    size_t dir_file_count;           // Number of regular files in this directory
    off_t dir_file_size;       // Cumulative size of regular files
    bool has_sibling[MAX_DEPTH]; // Used for printing tree branches │/└/├
} DirFrame;

// -------------------- NEW: visited directories linked list --------------------
typedef struct VisitedNode {
    dev_t st_dev;
    ino_t st_ino;
    struct VisitedNode *next;
} VisitedNode;

// Add a new visited directory node at head
VisitedNode* add_visited(VisitedNode **head, dev_t dev, ino_t ino) {
    VisitedNode *n = malloc(sizeof(VisitedNode));
    if (!n) { perror("malloc"); exit(EXIT_FAILURE); }
    n->st_dev = dev;
    n->st_ino = ino;
    n->next = *head;
    *head = n;
    return n;
}

// Check if a directory was visited before
bool visited_before(VisitedNode *head, dev_t dev, ino_t ino) {
    for (VisitedNode *cur = head; cur; cur = cur->next)
        if (cur->st_dev == dev && cur->st_ino == ino)
            return true;
    return false;
}

// Free visited list
void free_visited(VisitedNode *head) {
    VisitedNode *cur = head, *next;
    while (cur) { next = cur->next; free(cur); cur = next; }
}
// -------------------------------------------------------------------------------

// Free a linked list of SubDirNode
void free_subdirs(SubDirNode *head) {
    SubDirNode *cur = head, *next;
    while (cur) { 
        next = cur->next; 
        free(cur);        // Free current node
        cur = next;       // Move to next
    }
}

// Convert byte count to human-readable form (e.g. 1.2 K, 5.3 M, 1.0 G)
static void human_size(off_t bytes, char *out, size_t outsz)
{
    const char *units[] = {"B", "K", "M", "G", "T"};
    double size = (double)bytes;
    int u = 0;

    while (size >= 1024.0 && u < 4) {
        size /= 1024.0;
        u++;
    }
    snprintf(out, outsz, "%.1f%s", size, units[u]);
}

// Print a line representing a directory/subdirectory
void print_directory_line(const char* basePath, int depth, bool is_last,
                          bool has_sibling[], bool is_symdir, const char* symPath, 
                          size_t fc, off_t fs, bool is_recursive)
{
    const char* name;
    const char *slash = strrchr(basePath, '/');
    name = (slash && depth > 0) ? slash + 1 : basePath;

    // Print indentation / branches
    for (int i = 1; i < depth; i++)
        printf("%s", has_sibling[i] ? "│   " : "    ");

    if (depth > 0) {
        printf("%s── ", is_last ? "└" : "├");
    }

    if (is_symdir) {
        printf("@%s -> %s%s\n", name, symPath, is_recursive ? " [recursive - not followed]" : "");
        return;
    }

    if(fc>0){
		char hsize[32];
		human_size(fs, hsize, sizeof(hsize));
    	printf("%s [count: %zu] [size: %s]%s\n", name, fc, hsize, is_recursive ? " [recursive - not followed]" : "");
    } 
    else {
    	printf("%s%s\n", name, is_recursive ? " [recursive - not followed]" : "");
	}
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Stack to simulate recursion
    DirFrame *stack[MAX_DEPTH];
    int sp = 0; // Stack pointer

    // -------------------- NEW: visited directories head --------------------
    VisitedNode *visited_root = NULL;
    // -----------------------------------------------------------------------

    // Create the root directory frame
    DirFrame *root = malloc(sizeof(DirFrame));
    if (!root) { perror("malloc"); return EXIT_FAILURE; }
    snprintf(root->path, PATH_MAX, "%s", argv[1]);
    root->depth = 0;
    memset(root->has_sibling, 0, sizeof(root->has_sibling));
    root->dir = opendir(argv[1]); // Open root directory
    if (!root->dir) { perror("opendir"); free(root); return EXIT_FAILURE; }
    root->subdirs = NULL;          // No subdirectories collected yet
    root->current = NULL;          // Pointer to current subdirectory
    root->dir_file_count = 0;
    root->dir_file_size = 0;

    // -------------------- NEW: mark root as visited --------------------
    struct stat st_root;
    if (stat(root->path, &st_root) == 0)
        add_visited(&visited_root, st_root.st_dev, st_root.st_ino);
    // -------------------------------------------------------------------

    stack[sp++] = root;  // Push root frame onto stack

    // Main loop: process stack until empty
    size_t TOTAL_file_count=0;           // Total Number of regular files
    off_t TOTAL_file_size=0;       // Cumulative size of all regular files

    while (sp > 0) {
        DirFrame *frame = stack[sp - 1];  // Peek top of stack

        // First time entering this directory: collect files and subdirectories
        if (!frame->subdirs) {
            struct dirent *entry;
            struct stat st, lst;
            char buf[PATH_MAX];
            SubDirNode *head = NULL, *tail = NULL;

            // Initialize file counters for this frame
            frame->dir_file_count = 0;
            frame->dir_file_size = 0;

            // Read each entry in the directory
            while ((entry = readdir(frame->dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                    continue;  // Skip . and ..

                if (snprintf(buf, PATH_MAX, "%s/%s", frame->path, entry->d_name) >= PATH_MAX)
                    continue;  // Skip if path too long

                // Get lstat to check for symlink, stat for actual type
                if (lstat(buf, &lst) == -1) continue;
                if (stat(buf, &st) == -1) continue;

                // Count regular files and accumulate size
                if (S_ISREG(st.st_mode)) {
                    frame->dir_file_count++;
                    frame->dir_file_size += st.st_size;
                    TOTAL_file_count += frame->dir_file_count;
                    TOTAL_file_size += frame->dir_file_size;
                }

                bool is_symdir = false;
                if (S_ISLNK(lst.st_mode) && S_ISDIR(st.st_mode))
                    is_symdir = true;  // Symlink pointing to directory

                if (S_ISDIR(st.st_mode) || is_symdir) {
                    // Allocate node for this subdirectory
                    SubDirNode *n = malloc(sizeof(SubDirNode));
                    if (!n) { perror("malloc"); free_subdirs(head); closedir(frame->dir); return EXIT_FAILURE; }
                    snprintf(n->path, PATH_MAX, "%s", buf);
                    n->is_symlink = is_symdir;
                    if (is_symdir) {
                        // Read target of symlink
                        ssize_t len = readlink(buf, n->sym_path, PATH_MAX - 1);
                        if (len != -1) n->sym_path[len] = '\0';
                        else n->sym_path[0] = '\0';
                    } else n->sym_path[0] = '\0';
                    n->next = NULL;
                    if (!head) head = n;       // First node
                    else tail->next = n;        // Append to linked list
                    tail = n;
                }
            }

            frame->subdirs = head;   // Save subdirectory list in frame
            frame->current = head;   // Start processing first subdirectory

            // Print this directory now that counts are ready
            print_directory_line(frame->path, frame->depth, 
                                 frame->depth == 0 ? false : (frame->has_sibling[frame->depth] == 0), 
                                 frame->has_sibling, false, NULL, 
                                 frame->dir_file_count, frame->dir_file_size, false);
        }

        // Process next subdirectory in the list
        if (frame->current) {
            SubDirNode *cur = frame->current;
            frame->current = cur->next; // Move to next for next iteration
            bool is_last = (frame->current == NULL); // Check if this is last subdirectory
            frame->has_sibling[frame->depth + 1] = !is_last; // Track if siblings remain

                // -------------------- NEW: check visited list --------------------
				struct stat st_cur;
				if (stat(cur->path, &st_cur) == 0) {
					if (visited_before(visited_root, st_cur.st_dev, st_cur.st_ino)) {
						// Already visited: print loop detected and skip descending
						print_directory_line(cur->path, frame->depth + 1, is_last,
											 frame->has_sibling, cur->is_symlink, cur->sym_path,
											 0, 0, true);
//						printf("<< recursive loop detected from %s >>\n", cur->path); // visually mark it
						continue; // skip pushing onto stack
					} else {
						add_visited(&visited_root, st_cur.st_dev, st_cur.st_ino);
					}
				}
                // -----------------------------------------------------------------

                // Push new frame for this subdirectory (non-symlink)
                DirFrame *child = malloc(sizeof(DirFrame));
                if (!child) { perror("malloc"); return EXIT_FAILURE; }
                snprintf(child->path, PATH_MAX, "%s", cur->path);
                child->depth = frame->depth + 1;
                memcpy(child->has_sibling, frame->has_sibling, sizeof(frame->has_sibling));
                child->dir = opendir(cur->path); // Open subdirectory
                if (!child->dir) { perror("opendir"); free(child); continue; }
                child->subdirs = NULL;           // No subdirectories yet
                child->current = NULL;
                child->dir_file_count = 0;
                child->dir_file_size = 0;
                
                // --- NEW: depth guard ---
                if (sp >= MAX_DEPTH) {
                    fprintf(stderr, "Exceeded max depth %d at %s\n", MAX_DEPTH, cur->path);
                    closedir(child->dir);
                    free(child);
                    continue;
                }                
                stack[sp++] = child;             // Push onto stack
        } else {
            // Finished processing all subdirectories in this directory
            closedir(frame->dir);               // Close directory
            free_subdirs(frame->subdirs);       // Free linked list
            free(frame);                         // Free frame itself
            sp--;                                 // Pop stack
        }
    }

    // -------------------- NEW: free visited list --------------------
    free_visited(visited_root);
    // -------------------------------------------------------------------

    // -------------------- print totals --------------------
    char hsize[32];
    human_size(TOTAL_file_size, hsize, sizeof(hsize));
    printf("Total Number of Files: %zu\nTotal File Size: %s\n", TOTAL_file_count, hsize);

    return 0;  // Success
}
