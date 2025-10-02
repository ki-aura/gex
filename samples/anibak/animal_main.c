#include "animal_main.h"
#include "animal_ext.h"

RB_HEAD(AnimalTree_key, Animal) keyHead;
RB_HEAD(AnimalTree_name, Animal) nameHead;


/* -------------------------- Global Trees -------------------------- */
struct AnimalTree_key keyHead;
struct AnimalTree_name nameHead;

/* -------------------------- Wrappers and Size -------------------------- */
 int tree_size_key = 0;
 int tree_size_name = 10;


struct Animal *RB_INSERT_KEY(struct AnimalTree_key *head, struct Animal *node) {
    struct Animal *res = AnimalTree_key_RB_INSERT(head, node);
    if (res == NULL) tree_size_key++;
    return res;
}

struct Animal *RB_INSERT_NAME(struct AnimalTree_name *head, struct Animal *node) {
    struct Animal *res = AnimalTree_name_RB_INSERT(head, node);
    if (res == NULL) tree_size_name++;
    return res;
}

struct Animal *RB_DELETE_KEY(struct AnimalTree_key *head, struct Animal *node) {
    struct Animal *res = AnimalTree_key_RB_REMOVE(head, node);
    if (res != NULL) tree_size_key--;
    return res;
}

struct Animal *RB_DELETE_NAME(struct AnimalTree_name *head, struct Animal *node) {
    struct Animal *res = AnimalTree_name_RB_REMOVE(head, node);
    if (res != NULL) tree_size_name--;
    return res;
}

/*
int RB_SIZE(struct AnimalTree_key *head) {
    (void)head;  // head unused; size is global 
    return tree_size_key;
}
*/

/* ---------------------- Generate RB-tree Functions ---------------------- */

RB_GENERATE_STATIC(AnimalTree_key, Animal, by_key, Animal_Keycmp)
RB_GENERATE_STATIC(AnimalTree_name, Animal, by_name, Animal_Strcmp)

/* -------------------------- Main Program -------------------------- */

int main(void) {
    /* Initialize the global trees at runtime */
    RB_INIT(&keyHead);
    RB_INIT(&nameHead);

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

        if (RB_INSERT(AnimalTree_key, &keyHead, p) != NULL ||
            RB_INSERT(AnimalTree_name, &nameHead, p) != NULL) {
            fprintf(stderr, "Duplicate key or name: %d, %s\n", p->key, p->name);
            free(p);
        }
    }

    /* Find example */
    struct Animal search = {.key = 3};
    found = RB_FIND(AnimalTree_key, &keyHead, &search);
    if (found)
        printf("Found by key %d -> %s\n", found->key, found->name);

    /* Iterate forward by key */
    printf("All animals by key:\n");
    RB_FOREACH(p, AnimalTree_key, &keyHead) {
        printf("  %d -> %s\n", p->key, p->name);
    }

    /* Call external test */
    test_ext();

    /* Cleanup */
    while ((p = RB_MIN(AnimalTree_key, &keyHead)) != NULL) {
        RB_DELETE(AnimalTree_key, &keyHead, p);
        RB_DELETE(AnimalTree_name, &nameHead, p);
        free(p);
    }

    return 0;
}
