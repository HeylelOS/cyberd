#include "../log.h"
#include "daemons.h"

#include <stdlib.h>
#include <string.h>

#define daemons_node_height(node)  ((node) == NULL ? 0 : (node)->height)
#define daemons_node_balance(node) (daemons_node_height((node)->right) - daemons_node_height((node)->left))
#define max(a, b) ((a) > (b) ? (a) : (b))

static struct daemons_node *
daemons_node_rotate_left_left(struct daemons_node *node) {
	struct daemons_node *node2 = node->left;

	node->left = node2->right;
	node2->right = node;

	node->height = max(daemons_node_height(node->left), daemons_node_height(node->right)) + 1;
	node2->height = max(daemons_node_height(node2->left), node->height) + 1;

	return node2;
}

static struct daemons_node *
daemons_node_rotate_right_right(struct daemons_node *node) {
	struct daemons_node *node2 = node->right;

	node->right = node2->left;
	node2->left = node;

	node->height = max(daemons_node_height(node->left), daemons_node_height(node->right)) + 1;
	node2->height = max(daemons_node_height(node2->left), node->height) + 1;

	return node2;
}

static struct daemons_node *
daemons_node_rotate_left_right(struct daemons_node *node) {

	node->left = daemons_node_rotate_right_right(node->left);

	return daemons_node_rotate_left_left(node);
}

static struct daemons_node *
daemons_node_rotate_right_left(struct daemons_node *node) {

	node->right = daemons_node_rotate_left_left(node->right);

	return daemons_node_rotate_right_right(node);
}

static struct daemons_node *
daemons_node_insert(struct daemons_node *root,
	struct daemons_node *node) {

	if(root == NULL) {

		return node;
	} else {

		if(root->daemon.hash < node->daemon.hash) {

			root->left = daemons_node_insert(root->left, node);
			if(daemons_node_balance(root) == -2) {

				if(node->daemon.hash < root->left->daemon.hash) {
					root = daemons_node_rotate_left_left(root);
				} else {
					root = daemons_node_rotate_left_right(root);
				}
			}

		} else if(root->daemon.hash > node->daemon.hash) {

			root->right = daemons_node_insert(root->right, node);

			if(daemons_node_balance(root) == 2) {

				if(node->daemon.hash > root->left->daemon.hash) {
					root = daemons_node_rotate_right_right(root);
				} else {
					root = daemons_node_rotate_right_left(root);
				}
			}
		} else {
			log_print("Trying to reinsert daemon \"%s\"\n", node->daemon.name);
		}

		root->height = max(daemons_node_height(root->left), daemons_node_height(root->right)) + 1;

		return root;
	}
}

struct daemons_node *
daemons_node_create(const char *name) {
	struct daemons_node *node = malloc(sizeof(*node));

	daemon_init(&node->daemon, name);

	node->left = NULL;
	node->right = NULL;
	node->height = 0;

	return node;
}

void
daemons_node_destroy(struct daemons_node *node) {

	if(node != NULL) {

		log_print("Destroying daemon \"%s\" %.8X\n",
			node->daemon.name, node->daemon.hash);

		daemon_destroy(&node->daemon);

		daemons_node_destroy(node->left);
		daemons_node_destroy(node->right);

		free(node);
	}
}

void
daemons_init(struct daemons *daemons) {

	daemons->root = NULL;
}

void
daemons_destroy(struct daemons *daemons) {

	daemons_node_destroy(daemons->root);
}

void
daemons_insert(struct daemons *daemons,
	struct daemons_node *node) {

	daemons->root = daemons_node_insert(daemons->root, node);
}

struct daemon *
daemons_find(struct daemons *daemons,
	hash_t hash) {
	struct daemons_node *current = daemons->root;

	while(current != NULL
		&& current->daemon.hash != hash) {

		if(current->daemon.hash < hash) {
			current = current->left;
		} else { /* current->daemon.hash > hash */
			current = current->right;
		}
	}

	return current == NULL ? NULL : &current->daemon;
}

