#include "tree.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

#define tree_class_compare_hashes(lhs, rhs) ((int)((lhs) - (rhs)))
#define tree_node_height(node)  ((node) == NULL ? 0 : (node)->height)
#define tree_node_balance(node) ((node) == NULL ? 0 : \
	(tree_node_height((node)->right) - tree_node_height((node)->left)))
#define max(a, b) ((a) > (b) ? (a) : (b))

static int
tree_class_compare(const struct tree_class *class,
	const tree_element_t *lhs,
	const tree_element_t *rhs) {
	hash_t const lhash = class->hash_function(lhs),
		rhash = class->hash_function(rhs);

	return lhash == rhash ?
		class->compare_function(lhs, rhs)
		: tree_class_compare_hashes(lhash, rhash);
}

static struct tree_node *
tree_node_rotate_right(struct tree_node *node) {
	struct tree_node *node2 = node->left;

	node->left = node2->right;
	node2->right = node;

	node->height = max(tree_node_height(node->left),
		tree_node_height(node->right)) + 1;
	node2->height = max(tree_node_height(node2->left),
		node->height) + 1;

	return node2;
}

static struct tree_node *
tree_node_rotate_left(struct tree_node *node) {
	struct tree_node *node2 = node->right;

	node->right = node2->left;
	node2->left = node;

	node->height = max(tree_node_height(node->left),
		tree_node_height(node->right)) + 1;
	node2->height = max(tree_node_height(node2->right),
		node->height) + 1;

	return node2;
}

static struct tree_node *
tree_node_rotate_left_right(struct tree_node *node) {

	node->left = tree_node_rotate_left(node->left);

	return tree_node_rotate_right(node);
}

static struct tree_node *
tree_node_rotate_right_left(struct tree_node *node) {

	node->right = tree_node_rotate_right(node->right);

	return tree_node_rotate_left(node);
}

static struct tree_node *
tree_node_insert(struct tree_node *root,
	struct tree_node *node,
	const struct tree_class *class) {

	if(root == NULL) {

		return node;
	} else {
		int const comparison = tree_class_compare(class, node->element, root->element);

		if(comparison < 0) {

			root->left = tree_node_insert(root->left, node, class);

			if(tree_node_balance(root) == -2) {

				if(tree_class_compare(class, node->element, root->left->element) < 0) {
					root = tree_node_rotate_right(root);
				} else {
					root = tree_node_rotate_left_right(root);
				}
			}

		} else if(comparison > 0) {

			root->right = tree_node_insert(root->right, node, class);

			if(tree_node_balance(root) == 2) {

				if(tree_class_compare(class, node->element, root->right->element) > 0) {
					root = tree_node_rotate_left(root);
				} else {
					root = tree_node_rotate_right_left(root);
				}
			}
		} else {
			/* This should not happen, will certainly generate memory leaks
			in most use case in the process */
			log_error("Trying to reinsert element %lu", class->hash_function(node->element));
		}

		root->height = max(tree_node_height(root->left), tree_node_height(root->right)) + 1;

		return root;
	}
}

/*
 * Returns the root of the tree
 * and the removed node in removed,
 * left unchanged if the node doesn't exist
 */
static struct tree_node *
tree_node_remove_by_hash(struct tree_node *root,
	hash_t hash,
	const struct tree_class *class,
	struct tree_node **removed) {

	if(root != NULL) {
		int const comparison = tree_class_compare_hashes(hash, class->hash_function(root->element));

		if(comparison < 0) {
			root->left = tree_node_remove_by_hash(root->left, hash, class, removed);
		} else if(comparison > 0) {
			root->right = tree_node_remove_by_hash(root->right, hash, class, removed);
		} else {
			*removed = root;

			if(root->left != NULL
				&& root->right != NULL) {
				struct tree_node *successor = root->right;

				while(successor->left != NULL) {
					successor = successor->left;
				}

				root->right = tree_node_remove_by_hash(root->right,
					class->hash_function(successor->element), class, &successor);

				successor->left = root->left;
				successor->right = root->right;
				root = successor;
			} else if(root->left == NULL) {
				root = root->right;
			} else {
				root = root->left;
			}

			(*removed)->left = NULL;
			(*removed)->right = NULL;
			(*removed)->height = 1;
		}
	}

	if(root != NULL) {
		root->height = max(tree_node_height(root->left),
			tree_node_height(root->right)) + 1;

		if(tree_node_balance(root) > 1) {
			if(tree_node_balance(root->right) >= 0) {
				root = tree_node_rotate_left(root);
			} else {
				root = tree_node_rotate_right_left(root);
			}
		} else if(tree_node_balance(root) < -1) {
			if(tree_node_balance(root->left) <= 0) {
				root = tree_node_rotate_right(root);
			} else {
				root = tree_node_rotate_left_right(root);
			}
		}
	}

	return root;
}

struct tree_node *
tree_node_create(tree_element_t *element) {
	struct tree_node *node = malloc(sizeof(*node));

	if(node != NULL) {
		node->element = element;
		node->left = NULL;
		node->right = NULL;
		node->height = 1;
	}

	return node;
}

void
tree_node_destroy(struct tree_node *node) {

	if(node != NULL) {

		tree_node_destroy(node->left);
		tree_node_destroy(node->right);

		free(node);
	}
}

void
tree_init(struct tree *tree,
	const struct tree_class *class) {

	tree->class = class;
	tree->root = NULL;
}

void
tree_deinit(struct tree *tree) {

	tree_node_destroy(tree->root);
}

void
tree_insert(struct tree *tree,
	struct tree_node *node) {

	tree->root = tree_node_insert(tree->root, node, tree->class);
}

struct tree_node *
tree_remove_by_hash(struct tree *tree,
	hash_t hash) {
	struct tree_node *removed = NULL;

	tree->root = tree_node_remove_by_hash(tree->root, hash,
		tree->class, &removed);

	return removed;
}

tree_element_t *
tree_find_by_hash(struct tree *tree,
	hash_t hash) {
	struct tree_node *current = tree->root;
	hash_t currenthash;

	while(current != NULL
		&& (currenthash = tree->class->hash_function(current->element), currenthash != hash)) {

		if(hash < currenthash) {
			current = current->left;
		} else { /* currenthash < hash */
			current = current->right;
		}
	}

	return current == NULL ? NULL : current->element;
}

