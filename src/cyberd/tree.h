/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef TREE_H
#define TREE_H

/** Tree element. */
typedef void tree_element_t;

/** Tree node. */
struct tree_node;

/** Balanced sorted binary tree, implemented as an AVL. */
struct tree {
	int (* const compare)(const tree_element_t *, const tree_element_t *); /**< Function used to compare nodes' elements. */
	struct tree_node *root; /**< Root node. */
};

/**
 * Deinitialize and free a tree's memory.
 * @param tree Tree to deinitialize.
 */
static inline void
tree_deinit(struct tree *tree) {
	extern void tree_node_destroy(struct tree_node *node);

	tree_node_destroy(tree->root);
}

/**
 * Execute a traversal of the tree to mutate elements.
 * @param tree Tree to mutate.
 * @param mutate Mutation callback.
 */
static inline void
tree_mutate(struct tree *tree, void (* const mutate)(tree_element_t *element)) {
	extern void tree_node_preorder_mutate(struct tree_node *node, void (* const mutate)(tree_element_t *element));

	tree_node_preorder_mutate(tree->root, mutate);
}

/**
 * Insert a new node in a tree.
 * @param tree Tree to insert in.
 * @param node Node to insert.
 */
static inline void
tree_insert(struct tree *tree, tree_element_t *element) {
	extern struct tree_node * tree_node_create(tree_element_t *element);
	extern void tree_node_insert(struct tree_node **rootp, struct tree_node *node, int (* const compare)(const tree_element_t *, const tree_element_t *));

	tree_node_insert(&tree->root, tree_node_create(element), tree->compare);
}

tree_element_t *
tree_remove(struct tree *tree, const tree_element_t *element);

tree_element_t *
tree_find(struct tree *tree, const tree_element_t *element);

tree_element_t *
tree_last(struct tree *tree);

/* TREE_H */
#endif
