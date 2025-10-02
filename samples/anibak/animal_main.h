#ifndef ANIMAL_MAIN_H
#define ANIMAL_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"   /* BSD-style sys/tree.h, vendored for portability */

/* -------------------------- Node Definition -------------------------- */

struct Animal {
    RB_ENTRY(Animal) by_key;    /* link field for key-ordered tree */
    RB_ENTRY(Animal) by_name;   /* link field for name-ordered tree */
    int key;
    const char *name;
};

/* -------------------------- Tree Heads (globals) -------------------------- */

extern struct AnimalTree_key keyHead;
extern struct AnimalTree_name nameHead;

/* ---------------------- RB Prototype Static ---------------------- */

RB_PROTOTYPE_STATIC(AnimalTree_key, Animal, by_key, Animal_Keycmp)
RB_PROTOTYPE_STATIC(AnimalTree_name, Animal, by_name, Animal_Strcmp)


/* ---------------------- Comparison Functions ---------------------- */

static inline int Animal_Keycmp(struct Animal *a, struct Animal *b) {
    if (a->key < b->key) return -1;
    if (a->key > b->key) return 1;
    return 0;
}

static inline int Animal_Strcmp(struct Animal *a, struct Animal *b) {
    return strcmp(a->name, b->name);
}


/* ---------------------- RB Function Prototypes & Wrappers ---------------------- */

/* Prototype wrapper functions */
struct Animal *RB_INSERT_WRAPPER(struct AnimalTree_key *head, struct Animal *node);
struct Animal *RB_DELETE_WRAPPER(struct AnimalTree_key *head, struct Animal *node);
int RB_SIZE(struct AnimalTree_key *head);

/* Undef / redefine macros to use wrappers */
#undef RB_INSERT
#undef RB_REMOVE

extern struct Animal *RB_INSERT_KEY(struct AnimalTree_key *head, struct Animal *node);
extern struct Animal *RB_INSERT_NAME(struct AnimalTree_name *head, struct Animal *node);
extern struct Animal *RB_DELETE_KEY(struct AnimalTree_key *head, struct Animal *node);
extern struct Animal *RB_DELETE_NAME(struct AnimalTree_name *head, struct Animal *node);

#define RB_INSERT(headType, head, node) \
    _Generic((head), \
        struct AnimalTree_key *: RB_INSERT_KEY, \
        struct AnimalTree_name *: RB_INSERT_NAME \
    )(head, node)

#define RB_DELETE(headType, head, node) \
    _Generic((head), \
        struct AnimalTree_key *: RB_DELETE_KEY, \
        struct AnimalTree_name *: RB_DELETE_NAME \
    )(head, node)

extern int tree_size_key;
extern int tree_size_name;
    
#define RB_SIZE(head) \
    _Generic((head), \
        struct AnimalTree_key *: tree_size_key, \
        struct AnimalTree_name *: tree_size_name \
    )

#endif /* ANIMAL_TREE_H */
