#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir (POSIX)
#include <string.h>     // For strncpy, strcmp, strrchr, memset, memcpy, snprintf
#include <sys/stat.h>   // For struct stat, lstat, stat, S_ISDIR, S_ISLNK (POSIX)
#include <libgen.h>     // For basename if needed (not used here)
#include <unistd.h>     // For readlink (POSIX)
#include <inttypes.h>   // For intmax_t
#include "memsafe.h"
#include "gtree.h"
#include "visit_hash.h"
#include "khashl.h"

// ------------------- Visited Hash SetUp ------------------
typedef struct VisitedHash {
    dev_t st_dev;               // Device ID (unique per filesystem)
    ino_t st_ino;               // Inode number (unique per file on a filesystem)
} VisitedHash;

// Signature required by khashl.h: (KeyType a, KeyType b) -> bool (or int)
static inline int dev_ino_equal(VisitedHash a, VisitedHash b) {
    // khashl uses 'int' for the return, where 0 is FALSE and non-zero is TRUE.
    return (a.st_dev == b.st_dev) && (a.st_ino == b.st_ino);
}

static inline khint_t dev_ino_hash(VisitedHash k) {
    // A. Combine dev_t and ino_t into a 64-bit value (as a unique identifier).
    // The FNV-1a approach from before is one of the safest for this combination.
    // FNV_PRIME = 1099511621165ULL, FNV_OFFSET = 14695981039346656037ULL
    // For simplicity, let's use the standard multiplication/XOR combination first.

    uint64_t udev = (uint64_t)k.st_dev;
    uint64_t uino = (uint64_t)k.st_ino;

    // Use a multiplier to ensure a better distribution of the dev field.
    // This creates a 64-bit combined value.
    uint64_t combined_64 = (udev * 2654435761ULL) ^ uino;

    // B. Fold the 64-bit value down to the required 32-bit khint_t.
    // This is crucial to avoid losing information.
    khint_t hash_32 = (khint_t)(combined_64) ^ (khint_t)(combined_64 >> 32);

    return hash_32;
}

// Use a unique name for the generated type and a unique prefix.
// The macro will define a structure named 'kh_visited_set_t' (from the second parameter).
KHASHL_SET_INIT(static kh_inline klib_unused, visited_set, visited_set, VisitedHash, dev_ino_hash, dev_ino_equal)

typedef visited_set vnode_set_t; // Use the generated type name
vnode_set_t *nhash = NULL;

// ------------------- Visited hash functions ------------------
void create_visited_node_hash() {
	nhash = visited_set_init();
}

int add_visited(dev_t dev, ino_t ino) {
	VisitedHash key = { .st_dev = dev, .st_ino = ino };
	int absent;
	// The put function is prefix_put, which is visited_set_put
	visited_set_put(nhash, key, &absent);
	// absent == 0: Key already existed (visited before).
	// absent == 1: Key is new and inserted.
	return absent;
}


// Checks if a directory (identified by its unique dev/ino pair) has been visited before.
bool visited_before(dev_t dev, ino_t ino) {
    VisitedHash key = { .st_dev = dev, .st_ino = ino };
    khint_t k = visited_set_get(nhash, key);
    return k != kh_end(nhash);
}

// Frees all memory used by the visited directories linked list.
void free_visited_node_hash() {
    visited_set_destroy(nhash);
}

