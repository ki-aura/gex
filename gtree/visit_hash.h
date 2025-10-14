#ifndef VISIT_HASH_H
#define VISIT_HASH_H

// -------------------- Loop Detection: visited directories linked list --------------------
// Stores inode/device ID pairs of all directories that have been successfully entered.
// Used to detect and avoid infinite loops when following symlinks.

void create_visited_node_hash();
void free_visited_node_hash();
int add_visited(dev_t dev, ino_t ino);
bool visited_before(dev_t dev, ino_t ino);

#endif  
