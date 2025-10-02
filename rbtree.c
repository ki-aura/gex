#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"


/* -------------------------- Global Trees -------------------------- */
struct FByteTree edits = RB_INITIALIZER(&edit_tree);
int FBtree_size = 0;

/* -------------------------- Wrappers and Size -------------------------- */

struct FByte *RB_INSERT_KEY(struct FByteTree *head, size_t offs, unsigned char byt) {
	struct FByte *node;
	node = malloc(sizeof(*node));
	node->offset = offs;
	node->byte = byt;

    struct FByte *res = FByteTree_RB_INSERT(head, node);
    if (res == NULL) FBtree_size++;
    return res;
}

struct FByte *RB_REMOVE_KEY(struct FByteTree *head, struct FByte *node) {
    struct FByte *res = FByteTree_RB_REMOVE(head, node);
    if (res != NULL) {
    	FBtree_size--;
    	free(node);
    }
    return res;
}

void RB_CLEAR_TREE(struct FByteTree *head) {
    struct FByte *p;
    while ((p = RB_MIN(FByteTree, head)) != NULL) {
        RB_REMOVE(FByteTree, head, p);
    }
}

int RB_SIZE(void) {
	return FBtree_size;
}
	
/* ---------------------- Generate RB-tree Functions ---------------------- */

int off_cmp(struct FByte *a, struct FByte *b) {
    if (a->offset < b->offset) return -1;
    if (a->offset > b->offset) return 1;
    return 0;
}

RB_GENERATE(FByteTree, FByte, fb_key, off_cmp)

   

