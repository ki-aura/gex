#include "rbtree_main.h"
#include "rbtree_ext.h"

/* -------------------------- Global Trees -------------------------- */
RB_HEAD(AnimalTree_key, Animal) keyHead;
int tree_size_key = 0;

/* -------------------------- Wrappers and Size -------------------------- */

struct Animal *RB_INSERT_KEY(struct AnimalTree_key *head, struct Animal *node) {
    struct Animal *res = AnimalTree_key_RB_INSERT(head, node);
    if (res == NULL) tree_size_key++;
    return res;
}

struct Animal *RB_REMOVE_KEY(struct AnimalTree_key *head, struct Animal *node) {
    struct Animal *res = AnimalTree_key_RB_REMOVE(head, node);
    if (res != NULL) tree_size_key--;
    return res;
}

void RB_CLEAR_TREE(struct AnimalTree_key *head) {
    struct Animal *p;
    while ((p = RB_MIN(AnimalTree_key, head)) != NULL) {
        RB_REMOVE(AnimalTree_key, head, p);
        free(p);
    }
}

int RB_SIZE(void) {return tree_size_key;}

/* ---------------------- Generate RB-tree Functions ---------------------- */

int Animal_Keycmp(struct Animal *a, struct Animal *b) {
    if (a->key < b->key) return -1;
    if (a->key > b->key) return 1;
    return 0;
}

RB_GENERATE(AnimalTree_key, Animal, by_key, Animal_Keycmp)

/* -------------------------- Main Program -------------------------- */

int main(void) {
    /* Initialize the global trees at runtime */
    RB_INIT(&keyHead);

    struct Animal *p, *found;
    int i;

    const char *names[] = {"Dog", "Cat", "Horse", "Mouse", "Elephant"};
    int keys[] = {5, 2, 8, 1, 3};

    /* Insert Nodes */
    for (i = 0; i < 5; i++) {
        p = malloc(sizeof(*p));
        if (!p) { perror("malloc"); return 1; }

        p->key = keys[i];
        p->name = names[i];

        if (RB_INSERT(AnimalTree_key, &keyHead, p) != NULL) {
            fprintf(stderr, "Duplicate key or name: %d, %s\n", p->key, p->name);
            free(p);
        }
    }

    /* Find example */
    struct Animal search = {.key = 3};
    found = RB_FIND(AnimalTree_key, &keyHead, &search);
    if (found)
        printf("Found by key %d -> %s\n", found->key, found->name);

  	/* Find the smallest node >= key (lower bound) */
    search.key = 4; // 4 doesn't exist; should return key 5
    found = RB_NFIND(AnimalTree_key, &keyHead, &search);
    if (found)
        printf("Next >= 4 by key: %d -> %s\n", found->key, found->name);

    /* ---------------------- Remove Node ---------------------- */
    search.key = 2;
    found = RB_FIND(AnimalTree_key, &keyHead, &search);
    if (found) {
        /* Must remove from BOTH trees before freeing */
        RB_REMOVE(AnimalTree_key, &keyHead, found);
        printf("Removed key %d / name %s\n", found->key, found->name);
        free(found);
    }

    /* Forward iteration by key using RB_FOREACH */
    printf("All animals by key:\n");
    RB_FOREACH(p, AnimalTree_key, &keyHead) {
        printf("  %d -> %s\n", p->key, p->name);
    }

    // Reverse iteration by name using RB_FOREACH_REVERSE 
    printf("All animals by name (reverse):\n");
    RB_FOREACH_REVERSE(p, AnimalTree_key, &keyHead) {
        printf("  %s -> %d\n", p->name, p->key);
    }

    /* size */
    printf("size of tree is %d\n", RB_SIZE());

    /* Call external test */
    test_ext();

    /* Manual forward iteration by key */
    printf("Manual iteration by key:\n");
    for (p = RB_MIN(AnimalTree_key, &keyHead);
         p != NULL;
         p = RB_NEXT(AnimalTree_key, &keyHead, p)) {
        printf("%d -> %s\n", p->key, p->name);
    }

    /* Manual reverse iteration by name */
    printf("Manual iteration (backwards):\n");
    for (p = RB_MAX( AnimalTree_key, &keyHead);
         p != NULL;
         p = RB_PREV( AnimalTree_key, &keyHead, p)) {
        printf("%s -> %d\n", p->name, p->key);
    }
    

    /* Cleanup */
    RB_CLEAR_TREE(&keyHead);
    printf("checking for empty %d\n", RB_EMPTY(&keyHead));
    printf("size of tree is %d\n", RB_SIZE());

    return 0;
}


