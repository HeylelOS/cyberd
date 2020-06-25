#include "spawns.h"
#include "log.h"
#include "tree.h"

#include <stdlib.h>
#include <sys/wait.h>

static int
spawns_compare_function(const tree_element_t *lhs, const tree_element_t *rhs);

static hash_t
spawns_hash_function(const tree_element_t *element);

static struct tree spawns;

static const struct tree_class spawns_tree_class = {
	.compare_function = spawns_compare_function,
	.hash_function = spawns_hash_function,
};

static int
spawns_compare_function(const tree_element_t *lhs, const tree_element_t *rhs) {
	return (char *)rhs - (char *)lhs;
}

static hash_t
spawns_hash_function(const tree_element_t *element) {
	const struct daemon *daemon = element;

	return daemon->pid;
}

void
spawns_init(void) {

	tree_init(&spawns, &spawns_tree_class);
}

static void
spawns_preorder_stop(struct tree_node *node) {

	if (node != NULL) {
		struct daemon *daemon = node->element;

		/* It's important to disallow daemons' restart when stopping */
		daemon->conf.start.load = 0;
		daemon->conf.start.reload = 0;
		daemon->conf.start.exitsuccess = 0;
		daemon->conf.start.exitfailure = 0;
		daemon->conf.start.killed = 0;
		daemon->conf.start.dumped = 0;

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
	int status;
	pid_t child;

	spawns_preorder_end(spawns.root);

	while ((child = wait(&status)) > 0) {
		struct daemon *daemon = spawns_retrieve(child);

		if (daemon != NULL) {
			daemon->state = DAEMON_STOPPED;

			log_print("spawns_end: '%s' (pid: %d) force-ended with status: %d",
				daemon->name, child, status);
		} else {
			log_print("spawns_end: Orphan %d force-ended", child);
		}
	}
}

void
spawns_record(struct daemon *daemon) {
	struct tree_node *node
		= tree_node_create(daemon);

	if (node != NULL) {
		tree_insert(&spawns, node);
	}
}

struct daemon *
spawns_retrieve(pid_t pid) {
	struct tree_node *node = tree_remove_by_hash(&spawns, pid);
	struct daemon *daemon = NULL;

	if (node != NULL) {
		daemon = node->element;
		tree_node_destroy(node);
	}

	return daemon;
}

