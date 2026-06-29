#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"
#include "gex_helper_funcs.h"

/* -------------------------- Global Trees -------------------------- */
struct edit_tree edits = RB_INITIALIZER(&edit_tree);
int edit_tree_size = 0;

/* -------------------------- Wrappers and Size -------------------------- */

struct FByte *RB_INSERT_FB(struct edit_tree *head, size_t offs,
			   unsigned char byt)
{
	struct FByte *node;
	node = xmalloc(sizeof (*node));
	node->offset = offs;
	node->byte = byt;

	struct FByte *res = edit_tree_RB_INSERT(head, node);
	if (res == NULL) {
		edit_tree_size++;
		return node;	// inserted successfully
	} else {
		free(node);	// duplicate key, free the new one
		return res;	// return existing node
	}
}

struct FByte *RB_REMOVE_FB(struct edit_tree *head, struct FByte *node)
{
	struct FByte *res = edit_tree_RB_REMOVE(head, node);
	if (res != NULL) {
		edit_tree_size--;
		free(res);
	}
	return res;
}

void RB_CLEAR_TREE(struct edit_tree *head)
{
	struct FByte *p;
	while ((p = RB_MIN(edit_tree, head)) != NULL) {
		RB_REMOVE_FB(head, p);
	}
}

int RB_SIZE(void)
{
	return edit_tree_size;
}

/* ---------------------- Generate RB-tree Functions ---------------------- */

int off_cmp(struct FByte *a, struct FByte *b)
{
	if (a->offset < b->offset)
		return -1;
	if (a->offset > b->offset)
		return 1;
	return 0;
}

RB_GENERATE(edit_tree, FByte, fb_key, off_cmp)
