#include "spawns.h"
#include "tree.h"

#include <stdlib.h>

static struct tree spawns;

static hash_t
spawns_hash_field(const tree_element_t *element) {
	const struct daemon *daemon = element;

	return daemon->pid;
}

void
spawns_init(void) {

	tree_init(&spawns, spawns_hash_field);
}

static void
spawns_preorder_stop(struct tree_node *node) {

	if (node != NULL) {
		struct daemon *daemon = node->element;

		daemon_stop(daemon);

		spawns_preorder_stop(node->left);
		spawns_preorder_stop(node->right);
	}
}

void
spawns_stop(void) {

	spawns_preorder_stop(spawns.root);
}

bool
spawns_empty(void) {

	return spawns.root == NULL;
}

static void
spawns_preorder_end(struct tree_node *node) {

	if (node != NULL) {
		struct daemon *daemon = node->element;

		daemon_end(daemon);

		spawns_preorder_end(node->left);
		spawns_preorder_end(node->right);
	}
}

void
spawns_end(void) {

	spawns_preorder_end(spawns.root);
}

void
spawns_record(struct daemon *daemon) {
	struct tree_node *node
		= tree_node_create(daemon);

	tree_insert(&spawns, node);
}

struct daemon *
spawns_retrieve(pid_t pid) {
	struct tree_node *node = tree_remove(&spawns, pid);
	struct daemon *daemon = NULL;

	if (node != NULL) {
		daemon = node->element;
		tree_node_destroy(node);
	}

	return daemon;
}

