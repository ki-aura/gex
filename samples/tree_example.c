/*
 * bptree_word_count.c
 *
 * Read whitespace-separated tokens from a file, insert them into a B+ tree
 * ordered alphabetically (strcmp), maintain a count per unique token,
 * and print tokens with counts in alphabetical order.
 *
 * B+ tree supports splitting of leaves and internal nodes. Leaves are linked
 * for fast in-order iteration.
 *
 * Change ORDER_MAX_KEYS to tune node capacity (e.g. 4 for testing, 32 for production).
 *
 * Compile:
 *   cc -std=c11 -O0 -g -Wall -Wextra -o bptree_word_count bptree_word_count.c
 *
 * Run:
 *   ./bptree_word_count input.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>


typedef struct BNode {
    int is_leaf;            /* 1 => leaf, 0 => internal */
    int num_keys;           /* number of keys currently */
    char **keys;            /* array of key pointers (strings). For leaves: owned strings. For internals: aliases */
    int *counts;            /* only used for leaves: counts per key */
    struct BNode **children;/* child pointers (size keys+1). For leaves unused (but allocated) */
    struct BNode *next;     /* leaf sibling link for iteration */
    struct BNode *prev;  	/* backward leaf link */
} BNode;

typedef struct {
    BNode *root;
} BTree;

#define ORDER_MAX_KEYS 16
#define KEYS_ARR_SIZE (ORDER_MAX_KEYS + 1)      /* allocate +1 for temporary overflow */
#define CHILDS_ARR_SIZE (ORDER_MAX_KEYS + 2)    /* children = keys+1; +1 for overflow */


/* Estimate ORDER_MAX_KEYS from file size (bytes) */
/* Tunable parameter: maximum keys per node before splitting */
static size_t estimate_order_max_keys(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        perror("stat");
        return 16;  // fallback default
    }

    size_t file_size = (size_t)st.st_size;
    if (file_size == 0) return 4;

    size_t est_words = file_size / 6; // average 6 bytes per word

    // Target tree height ~4–5
    size_t order = 4;
    while (order < 512 && order * order * order * order < est_words) { // 4th power
        order *= 2; // double until roughly height 4
    }

    // Clamp to reasonable min/max
    if (order < 4) order = 4;
    if (order > 512) order = 512;

    return order;
}

/* Utility: allocate node (leaf flag). Arrays sized with overflow space. */
static BNode *node_create(int is_leaf) {
    BNode *n = calloc(1, sizeof(BNode));
    if (!n) { perror("calloc"); exit(EXIT_FAILURE); }
    n->is_leaf = is_leaf ? 1 : 0;
    n->num_keys = 0;
    n->keys = calloc(KEYS_ARR_SIZE, sizeof(char*));
    n->children = calloc(CHILDS_ARR_SIZE, sizeof(BNode*));
    if (!n->keys || !n->children) { perror("calloc"); exit(EXIT_FAILURE); }
    if (is_leaf) {
        n->counts = calloc(KEYS_ARR_SIZE, sizeof(int));
        if (!n->counts) { perror("calloc"); exit(EXIT_FAILURE); }
    } else {
        n->counts = NULL;
    }
    n->next = NULL;
    n->prev = NULL;
    return n;
}

/* Create empty tree */
static BTree *tree_create(void) {
    BTree *t = malloc(sizeof(BTree));
    if (!t) { perror("malloc"); exit(EXIT_FAILURE); }
    t->root = node_create(1); /* start with single leaf */
    return t;
}

/* Free helper: free only leaf-owned strings; internal pointers are aliases */
static void node_free_recursive(BNode *n) {
    if (!n) return;

    if (n->is_leaf) {
        for (int i = 0; i < n->num_keys; ++i) {
            if (n->keys[i]) {
                free(n->keys[i]); /* leaf owns strings */
                n->keys[i] = NULL;
            }
        }
    } else {
        for (int i = 0; i <= n->num_keys; ++i) {
            if (n->children[i]) node_free_recursive(n->children[i]);
        }
    }

    free(n->keys);
    free(n->children);
    if (n->counts) free(n->counts);
    free(n);
}

static void tree_free(BTree *t) {
    if (!t) return;
    if (t->root) node_free_recursive(t->root);
    free(t);
}

/* Find first index i in node such that keys[i] >= key (strcmp), returns i in [0..num_keys] */
static int node_find_index(BNode *n, const char *key) {
    int i = 0;
    while (i < n->num_keys && strcmp(n->keys[i], key) < 0) i++;
    return i;
}

/* Split child node parent->children[idx]. Parent must have space for an extra key/child.
 * After split, parent gets one more key and child pointer.
 */
static void split_child(BNode *parent, int idx) {
    BNode *child = parent->children[idx];
    assert(child != NULL);

    BNode *newn = node_create(child->is_leaf);

    int total = child->num_keys;
    int mid = total / 2; /* split point */

    if (child->is_leaf) {
        int new_count = total - mid;
        for (int j = 0; j < new_count; ++j) {
            newn->keys[j] = child->keys[mid + j];    // move ownership
            newn->counts[j] = child->counts[mid + j];
            child->keys[mid + j] = NULL;
        }
        newn->num_keys = new_count;
        child->num_keys = mid;

        /* fix sibling links */
        newn->next = child->next;
        if (newn->next) newn->next->prev = newn;  // backward link from next leaf
        newn->prev = child;                       // backward link to left sibling
        child->next = newn;

        /* shift parent's children right */
        for (int j = parent->num_keys; j >= idx + 1; --j) {
            parent->children[j+1] = parent->children[j];
        }
        parent->children[idx+1] = newn;

        /* shift parent's keys and insert promotion key = first key of new leaf */
        for (int j = parent->num_keys - 1; j >= idx; --j) {
            parent->keys[j+1] = parent->keys[j];
        }
        parent->keys[idx] = newn->keys[0];  // alias only
        parent->num_keys++;
    } else {
        /* Internal node split: promote middle key */
        int right_count = total - (mid + 1);
        for (int j = 0; j < right_count; ++j) {
            newn->keys[j] = child->keys[mid + 1 + j];
        }
        for (int j = 0; j <= right_count; ++j) {
            newn->children[j] = child->children[mid + 1 + j];
        }
        newn->num_keys = right_count;
        child->num_keys = mid;

        /* shift parent's children and keys */
        for (int j = parent->num_keys; j >= idx + 1; --j) {
            parent->children[j+1] = parent->children[j];
        }
        parent->children[idx+1] = newn;

        for (int j = parent->num_keys - 1; j >= idx; --j) {
            parent->keys[j+1] = parent->keys[j];
        }
        parent->keys[idx] = child->keys[mid]; // alias only
        parent->num_keys++;
    }
}


/* Insert into a non-full node. The provided 'key' must be a strdup'd string if insertion into leaf occurs.
 * If a duplicate is found in a leaf, function increments the count and frees key.
 */
/* Check if key exists in leaf or its immediate next leaf first key, consolidate if found */
/* Return 1 if key exists (increment count), 0 otherwise.
 * Checks current leaf and boundary keys in prev/next leaves to prevent duplicates across splits.
 */
static int leaf_find_and_increment(BNode *leaf, const char *key) {
    // Check current leaf
    for (int i = 0; i < leaf->num_keys; ++i) {
        if (strcmp(leaf->keys[i], key) == 0) {
            leaf->counts[i]++;
            return 1;
        }
    }

    // Check previous leaf’s last key (possible duplicate across split)
    if (leaf->prev && leaf->prev->num_keys > 0) {
        int last = leaf->prev->num_keys - 1;
        if (strcmp(leaf->prev->keys[last], key) == 0) {
            leaf->prev->counts[last]++;
            return 1;
        }
    }

    // Check next leaf’s first key (possible duplicate across split)
    if (leaf->next && leaf->next->num_keys > 0) {
        if (strcmp(leaf->next->keys[0], key) == 0) {
            leaf->next->counts[0]++;
            return 1;
        }
    }

    return 0;  // key not found
}

/* Insert into a non-full node, consolidating duplicates across split leaf boundaries */
static void insert_nonfull(BNode *node, char *key) {
    if (node->is_leaf) {
        if (leaf_find_and_increment(node, key)) {
            free(key);
            return;
        }

        /* Find insert position */
        int i = node->num_keys - 1;
        while (i >= 0 && strcmp(node->keys[i], key) > 0) {
            node->keys[i+1] = node->keys[i];
            node->counts[i+1] = node->counts[i];
            i--;
        }
        int pos = i + 1;

        /* Insert new key */
        node->keys[pos] = key;
        node->counts[pos] = 1;
        node->num_keys++;
    } else {
        int i = node_find_index(node, key);
        BNode *child = node->children[i];
        assert(child != NULL);

        if ((size_t)child->num_keys == ORDER_MAX_KEYS) {
            split_child(node, i);
            /* After split, key may belong to new right child */
            if (strcmp(key, node->keys[i]) >= 0) i++;
        }

        insert_nonfull(node->children[i], key);
    }
}


/* Public insert */
static void btree_insert(BTree *t, const char *token) {
    if (!token || token[0] == '\0') return;
    char *dup = strdup(token);
    if (!dup) { perror("strdup"); exit(EXIT_FAILURE); }

    BNode *r = t->root;
    if ((size_t)r->num_keys == ORDER_MAX_KEYS) {
        BNode *s = node_create(0);  // new root
        s->children[0] = r;
        t->root = s;
        split_child(s, 0);
        insert_nonfull(s, dup);
    } else {
        insert_nonfull(r, dup);
    }
}


/* Iterate leaves (leftmost to right) and print key (count) */
static void btree_print(BTree *t) {
    if (!t || !t->root) return;
    BNode *n = t->root;
    while (!n->is_leaf) n = n->children[0];
    while (n) {
        for (int i = 0; i < n->num_keys; ++i) {
            printf("%s (%d)\n", n->keys[i], n->counts[i]);
        }
        n = n->next;
    }
}

static void btree_print_reverse(BTree *t) {
    if (!t || !t->root) return;

    /* go to rightmost leaf */
    BNode *n = t->root;
    while (!n->is_leaf) n = n->children[n->num_keys];

    /* iterate backward */
    while (n) {
        for (int i = n->num_keys - 1; i >= 0; --i) {
            printf("%s (%d)\n", n->keys[i], n->counts[i]);
        }
        n = n->prev;
    }
}

/* Read whitespace-separated tokens from file and insert */
static void process_file(BTree *t, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "fopen(%s): %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    char buf[4096];
    while (fscanf(f, "%4095s", buf) == 1) {
        if (buf[0] == '\0') continue;  // skip empty words
        btree_insert(t, buf);
    }
    if (ferror(f)) {
        perror("fscanf");
        fclose(f);
        exit(EXIT_FAILURE);
    }
    fclose(f);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 2;
    }

    BTree *tree = tree_create();
    process_file(tree, argv[1]);
    printf("Forwards...\n'n");
    btree_print(tree);
    printf("\n\nBackwards...\n\n");
    btree_print_reverse(tree);
    tree_free(tree);
    return 0;
}
