#ifndef TREE_H
#define TREE_H

#include "hash.h"

/*
 * tree.h is a generic AVL Tree
 * which contains an anonymous pointer,
 * for logn access according to
 * a field determined by a function
 */

/** Alias to what it holds, just to avoid void * for code clarity */
typedef void tree_element_t;

/** Tree class, supporting dynamic methods */
struct tree_class {
	int    (*compare_function)(const tree_element_t *, const tree_element_t *);
	hash_t (*hash_function)(const tree_element_t *);
};

/** Node in the tree */
struct tree_node {
	tree_element_t *element;

	struct tree_node *left, *right;
	int height;
};

/** The tree, with hash field accessor */
struct tree {
	const struct tree_class *class;
	struct tree_node *root;
};

/**
 * Creates a new node to hold element
 * @param element Element hold by the node
 * @return Newly allocated node, must be tree_node_destroy()'d
 */
struct tree_node *
tree_node_create(tree_element_t *element);

/**
 * Destroys a previously tree_node_create()'d node
 * @param node Node to destroy
 */
void
tree_node_destroy(struct tree_node *node);

/**
 * Initialize tree structure
 * @param tree Pointer to the structure to initialize
 * @param class Class associated to this tree
 */
void
tree_init(struct tree *tree,
	const struct tree_class *class);

/**
 * Frees informations of a tree
 * @param tree Tree to deinitialize
 */
void
tree_deinit(struct tree *tree);

/**
 * Insert a node in the tree
 * @param tree Tree in which to insert
 * @param node Node to insert
 */
void
tree_insert(struct tree *tree,
	struct tree_node *node);

/**
 * Remove a node from the tree
 * @param tree Tree to remove from
 * @param hash Hash of the (maybe) designated node
 * @return NULL if not found, or the node else, removed from tree
 */
struct tree_node *
tree_remove_by_hash(struct tree *tree,
	hash_t hash);

/**
 * Find a node by its hash
 * @param tree Tree to find in
 * @param hash Hash of the (maybe) designated node
 * @return NULL if not found, or the element else
 */
tree_element_t *
tree_find_by_hash(struct tree *tree,
	hash_t hash);

/* TREE_H */
#endif
