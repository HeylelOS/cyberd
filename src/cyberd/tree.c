/* SPDX-License-Identifier: BSD-3-Clause */
#include "tree.h"

#include <stdlib.h> /* malloc, free, abort */
#include <assert.h> /* assert */

/** Tree node. */
struct tree_node {
	tree_element_t *element; /**< Node associated element. */
	struct tree_node *left, *right; /**< Node children. */
	int height; /**< Height of the tree node, 1 if no children. */
};

static inline int
max(int a, int b) {
	return a > b ? a : b;
}

/*************
 * Tree node *
 *************/

static inline int
tree_node_height(const struct tree_node *node) {
	return node != NULL ? node->height : 0;
}

static inline int
tree_node_balance(const struct tree_node *node) {
	return node != NULL ? tree_node_height(node->right) - tree_node_height(node->left) : 0;
}

static void
tree_node_rotate_right(struct tree_node **nodep) {
	struct tree_node * const node = *nodep, * const node2 = node->left;

	node->left = node2->right;
	node2->right = node;

	node->height = max(tree_node_height(node->left), tree_node_height(node->right)) + 1;
	node2->height = max(tree_node_height(node2->left), node->height) + 1;

	*nodep = node2;
}

static void
tree_node_rotate_left(struct tree_node **nodep) {
	struct tree_node * const node = *nodep, * const node2 = node->right;

	node->right = node2->left;
	node2->left = node;

	node->height = max(tree_node_height(node->left), tree_node_height(node->right)) + 1;
	node2->height = max(tree_node_height(node2->right), node->height) + 1;

	*nodep = node2;
}

static void
tree_node_rotate_left_right(struct tree_node **nodep) {
	struct tree_node *node = *nodep;

	tree_node_rotate_left(&node->left);
	tree_node_rotate_right(&node);

	*nodep = node;
}

static void
tree_node_rotate_right_left(struct tree_node **nodep) {
	struct tree_node *node = *nodep;

	tree_node_rotate_right(&node->right);
	tree_node_rotate_left(&node);

	*nodep = node;
}

/**
 * Insert a new node in the tree using a custom comparison function.
 * @param[in,out] rootp Root of the tree, returns new root after insertion.
 * @param node New node to insert.
 * @param compare Comparison function used to compare node elements.
 */
void
tree_node_insert(struct tree_node **rootp, struct tree_node *node, int (* const compare)(const tree_element_t *, const tree_element_t *)) {
	struct tree_node *root = *rootp;

	if (root != NULL) {
		const int comparison = compare(node->element, root->element);

		if (comparison < 0) {

			tree_node_insert(&root->left, node, compare);

			if (tree_node_balance(root) == -2) {
				if (compare(node->element, root->left->element) < 0) {
					tree_node_rotate_right(&root);
				} else {
					tree_node_rotate_left_right(&root);
				}
			}
		} else if (comparison > 0) {

			tree_node_insert(&root->right, node, compare);

			if (tree_node_balance(root) == 2) {
				if (compare(node->element, root->right->element) > 0) {
					tree_node_rotate_left(&root);
				} else {
					tree_node_rotate_right_left(&root);
				}
			}
		} else {
			/* Trying to reinsert an element, which should never happen */
			abort();
		}

		root->height = max(tree_node_height(root->left), tree_node_height(root->right)) + 1;
	} else {
		root = node;
	}

	*rootp = root;
}

/**
 * Remove a node from a tree.
 * @param[in,out] rootp Root of the tree, returns new root after removal.
 * @param element Element identical with the one of the node we should remove (according to @p compare).
 * @param compare Comparison function used to compare node elements.
 * @returns Removed node, _NULL_ if none found.
 */
static struct tree_node *
tree_node_remove(struct tree_node **rootp, const tree_element_t *element, int (* const compare)(const tree_element_t *, const tree_element_t *)) {
	struct tree_node *root = *rootp, *removed;

	if (root != NULL) {
		const int comparison = compare(element, root->element);

		if (comparison < 0) {
			removed = tree_node_remove(&root->left, element, compare);
		} else if (comparison > 0) {
			removed = tree_node_remove(&root->right, element, compare);
		} else {
			removed = root;

			if (root->left != NULL && root->right != NULL) {
				struct tree_node *successor = root->right;

				while (successor->left != NULL) {
					successor = successor->left;
				}

				successor = tree_node_remove(&root->right, successor->element, compare);
				successor->left = root->left;
				successor->right = root->right;

				root = successor;

			} else if (root->left == NULL) {
				root = root->right;
			} else {
				root = root->left;
			}

			removed->left = NULL;
			removed->right = NULL;
			removed->height = 1;
		}
	} else {
		removed = NULL;
	}

	if (root != NULL) {
		root->height = max(tree_node_height(root->left), tree_node_height(root->right)) + 1;

		if (tree_node_balance(root) > 1) {
			if (tree_node_balance(root->right) >= 0) {
				tree_node_rotate_left(&root);
			} else {
				tree_node_rotate_right_left(&root);
			}
		} else if (tree_node_balance(root) < -1) {
			if (tree_node_balance(root->left) <= 0) {
				tree_node_rotate_right(&root);
			} else {
				tree_node_rotate_left_right(&root);
			}
		}
	}

	*rootp = root;

	return removed;
}

/**
 * Create a new tree node.
 * @param element Element associated with the node.
 * @returns New tree node, _NULL_ on error.
 */
struct tree_node *
tree_node_create(tree_element_t *element) {
	struct tree_node * const node = malloc(sizeof (*node));

	assert(node != NULL);

	node->element = element;
	node->left = NULL;
	node->right = NULL;
	node->height = 1;

	return node;
}

/**
 * Deallocates a tree node and all its children.
 * @param node Node to deallocate.
 */
void
tree_node_destroy(struct tree_node *node) {

	if (node != NULL) {
		tree_node_destroy(node->left);
		tree_node_destroy(node->right);
		free(node);
	}
}

/**
 * Execute a preorder traversal of the tree node and mutate its elements.
 * @param node Node to mutate and traverse.
 * @param mutate Mutation callback.
 */
void
tree_node_preorder_mutate(struct tree_node *node, void (* const mutate)(tree_element_t *element)) {

	if (node != NULL) {
		mutate(node->element);
		tree_node_preorder_mutate(node->left, mutate);
		tree_node_preorder_mutate(node->right, mutate);
	}
}

/********
 * Tree *
 ********/

/**
 * Remove a node from a tree according to an element.
 * @param tree Tree to remove from.
 * @param element Element identical with the element of the node we should remove.
 * @returns The removed element, identical with @p element, _NULL_ if none found.
 */
tree_element_t *
tree_remove(struct tree *tree, const tree_element_t *element) {
	struct tree_node * const node = tree_node_remove(&tree->root, element, tree->compare);

	if (node != NULL) {
		tree_element_t * const element = node->element;

		tree_node_destroy(node);

		return element;
	} else {
		return NULL;
	}
}

/**
 * Find a node from a tree according to a reference element.
 * @param tree Tree to search in.
 * @param element Element identical with the element of the node we should find.
 * @returns An element identical with @p element, _NULL_ if none found.
 */
tree_element_t *
tree_find(struct tree *tree, const tree_element_t *element) {
	struct tree_node *current = tree->root;
	int comparison;

	while (current != NULL && (comparison = tree->compare(element, current->element), comparison != 0)) {
		if (comparison < 0) {
			current = current->left;
		} else {
			assert(comparison > 0);
			current = current->right;
		}
	}

	return current != NULL ? current->element : NULL;
}

/**
 * Find the last node's element of the tree. The greatest according to the comparison function.
 * @param tree Tree to search in.
 * @returns The last tree node's element, _NULL_ if none found.
 */
tree_element_t *
tree_last(struct tree *tree) {
	struct tree_node *current = tree->root;

	if (current != NULL) {

		while (current->right != NULL) {
			current = current->right;
		}

		return current->element;
	} else {
		return NULL;
	}
}
