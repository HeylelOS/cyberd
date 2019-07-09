#include "tree.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

#define tree_node_height(node)  ((node) == NULL ? 0 : (node)->height)
#define tree_node_balance(node) ((node) == NULL ? 0 : \
	(tree_node_height((node)->right) - tree_node_height((node)->left)))
#define max(a, b) ((a) > (b) ? (a) : (b))

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
	hash_t (*hash_field)(const tree_element_t *)) {

	if (root == NULL) {

		return node;
	} else {

		if (hash_field(node->element)
			< hash_field(root->element)) {

			root->left = tree_node_insert(root->left, node, hash_field);

			if (tree_node_balance(root) == -2) {

				if (hash_field(node->element) < hash_field(root->left->element)) {
					root = tree_node_rotate_right(root);
				} else {
					root = tree_node_rotate_left_right(root);
				}
			}

		} else if (hash_field(node->element) > hash_field(root->element)) {

			root->right = tree_node_insert(root->right, node, hash_field);

			if (tree_node_balance(root) == 2) {

				if (hash_field(node->element) > hash_field(root->right->element)) {
					root = tree_node_rotate_left(root);
				} else {
					root = tree_node_rotate_right_left(root);
				}
			}
		} else {
			/* This should not happen, will certainly generate memory leaks
			in most use case in the process */
			log_error("Trying to reinsert element %lu", hash_field(node->element));
		}

		root->height = max(tree_node_height(root->left), tree_node_height(root->right)) + 1;

		return root;
	}
}

/*
 * Returns the root of the tree
 * and the removed node in removed,
 * left unchanged if the node dosn't exist
 */
static struct tree_node *
tree_node_remove(struct tree_node *root,
	hash_t hash,
	struct tree_node **removed,
	hash_t (*hash_field)(const tree_element_t *)) {

	if (root != NULL) {
		if (hash < hash_field(root->element)) {
			root->left = tree_node_remove(root->left, hash, removed, hash_field);
		} else if (hash > hash_field(root->element)) {
			root->right = tree_node_remove(root->right, hash, removed, hash_field);
		} else {
			*removed = root;

			if (root->left != NULL
				&& root->right != NULL) {
				struct tree_node *successor = root->right;

				while (successor->left != NULL) {
					successor = successor->left;
				}

				root->right = tree_node_remove(root->right,
					hash_field(successor->element), &successor, hash_field);

				successor->left = root->left;
				successor->right = root->right;
				root = successor;
			} else if (root->left == NULL) {
				root = root->right;
			} else {
				root = root->left;
			}

			(*removed)->left = NULL;
			(*removed)->right = NULL;
			(*removed)->height = 1;
		}
	}

	if (root != NULL) {
		root->height = max(tree_node_height(root->left),
			tree_node_height(root->right)) + 1;

		if (tree_node_balance(root) > 1) {
			if (tree_node_balance(root->right) >= 0) {
				root = tree_node_rotate_left(root);
			} else {
				root = tree_node_rotate_right_left(root);
			}
		} else if (tree_node_balance(root) < -1) {
			if (tree_node_balance(root->left) <= 0) {
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
	struct tree_node *node = malloc(sizeof (*node));

	node->element = element;
	node->left = NULL;
	node->right = NULL;
	node->height = 1;

	return node;
}

void
tree_node_destroy(struct tree_node *node) {

	if (node != NULL) {

		tree_node_destroy(node->left);
		tree_node_destroy(node->right);

		free(node);
	}
}

void
tree_init(struct tree *tree,
	hash_t (*hash_field)(const tree_element_t *)) {

	tree->root = NULL;
	tree->hash_field = hash_field;
}

void
tree_deinit(struct tree *tree) {

	tree_node_destroy(tree->root);
}

void
tree_insert(struct tree *tree,
	struct tree_node *node) {

	tree->root = tree_node_insert(tree->root, node,
		tree->hash_field);
}

struct tree_node *
tree_remove(struct tree *tree,
	hash_t hash) {
	struct tree_node *removed = NULL;

	tree->root = tree_node_remove(tree->root, hash,
		&removed, tree->hash_field);

	return removed;
}

tree_element_t *
tree_find(struct tree *tree,
	hash_t hash) {
	struct tree_node *current = tree->root;

	while (current != NULL
		&& tree->hash_field(current->element) != hash) {

		if (hash < tree->hash_field(current->element)) {
			current = current->left;
		} else { /* tree->hash_field(current->element) < hash */
			current = current->right;
		}
	}

	return current == NULL ? NULL : current->element;
}

