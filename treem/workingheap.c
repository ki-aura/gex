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
    size_t file_count;           // Number of regular files in this directory
    off_t total_file_size;       // Cumulative size of regular files
    bool has_sibling[MAX_DEPTH]; // Used for printing tree branches │/└/├
} DirFrame;

// Free a linked list of SubDirNode
void free_subdirs(SubDirNode *head) {
    SubDirNode *cur = head, *next;
    while (cur) { 
        next = cur->next; 
        free(cur);        // Free current node
        cur = next;       // Move to next
    }
}

// Print a line representing a directory/subdirectory
void print_directory_line(const char* basePath, int depth, bool is_last,
                          bool has_sibling[], bool is_symdir, const char* symPath, 
                          size_t fc, off_t fs)
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
        printf("@%s -> %s\n", name, symPath);
        return;
    }

    if(fc>0) printf("%s [count: %zu] [size: %jd]\n", name, fc, (intmax_t)fs);
    else printf("%s\n", name);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Stack to simulate recursion
    DirFrame *stack[MAX_DEPTH];
    int sp = 0; // Stack pointer

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
    root->file_count = 0;
    root->total_file_size = 0;

    stack[sp++] = root;  // Push root frame onto stack

    // Main loop: process stack until empty
    while (sp > 0) {
        DirFrame *frame = stack[sp - 1];  // Peek top of stack

        // First time entering this directory: collect files and subdirectories
        if (!frame->subdirs) {
            struct dirent *entry;
            struct stat st, lst;
            char buf[PATH_MAX];
            SubDirNode *head = NULL, *tail = NULL;

            // Initialize file counters for this frame
            frame->file_count = 0;
            frame->total_file_size = 0;

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
                    frame->file_count++;
                    frame->total_file_size += st.st_size;
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
                                 frame->file_count, frame->total_file_size);
        }

        // Process next subdirectory in the list
        if (frame->current) {
            SubDirNode *cur = frame->current;
            frame->current = cur->next; // Move to next for next iteration
            bool is_last = (frame->current == NULL); // Check if this is last subdirectory
            frame->has_sibling[frame->depth + 1] = !is_last; // Track if siblings remain

            if (!cur->is_symlink) {
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
                child->file_count = 0;
                child->total_file_size = 0;
                
				// --- NEW: depth guard ---
				if (sp >= MAX_DEPTH) {
					fprintf(stderr, "Exceeded max depth %d at %s\n", MAX_DEPTH, cur->path);
					closedir(child->dir);
					free(child);
					continue;
				}                
                stack[sp++] = child;             // Push onto stack
            } else {
                // Symlink: print immediately with count 0
                print_directory_line(cur->path, frame->depth + 1, is_last, frame->has_sibling,
                                     true, cur->sym_path, 0, 0);
            }
        } else {
            // Finished processing all subdirectories in this directory
            closedir(frame->dir);               // Close directory
            free_subdirs(frame->subdirs);       // Free linked list
            free(frame);                         // Free frame itself
            sp--;                                 // Pop stack
        }
    }
    return 0;  // Success
}
