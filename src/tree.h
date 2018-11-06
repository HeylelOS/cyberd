#ifndef TREE_H
#define TREE_H

#include "hash.h"

/**
 * tree.h is a generic AVL Tree
 * which contains an anonymous pointer,
 * for logn access according to
 * a field determined by a function
 */

typedef void tree_element_t;

struct tree_node {
	tree_element_t *element;

	struct tree_node *left, *right;
	int height;
};

struct tree {
	struct tree_node *root;
	hash_t (*hash_field)(const tree_element_t *);
};

struct tree_node *
tree_node_create(tree_element_t *element);

void
tree_node_destroy(struct tree_node *node);

void
tree_init(struct tree *tree,
	hash_t (*hash_field)(const tree_element_t *));

void
tree_deinit(struct tree *tree);

void
tree_insert(struct tree *tree,
	struct tree_node *node);

struct tree_node *
tree_remove(struct tree *tree,
	hash_t hash);

tree_element_t *
tree_find(struct tree *tree,
	hash_t hash);

/* TREE_H */
#endif
