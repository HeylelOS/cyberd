/* SPDX-License-Identifier: BSD-3-Clause */
#include "socket_switch.h"

#include "socket_endpoint_node.h"
#include "socket_node.h"

#include <stdlib.h> /* NULL */
#include <string.h> /* memcpy */
#include <sys/stat.h> /* mkdir */
#include <syslog.h> /* syslog */
#include <errno.h> /* errno, ... */

#include <assert.h> /* assert */

/**
 * Main socket nodes storage, where all socket nodes are allocated/operated.
 * This storage is index using the node's file descriptors as identifiers.
 */
static struct {
	struct tree snodes; /**< Socket node storage tree. */
	fd_set activeset; /**< Active sockets. */
	fd_set readset; /**< Exchange socket set. */
} socket_switch = {
	.snodes = { .compare = socket_nodes_compare },
};

/** Create the first communication endpoint. */
void
socket_switch_setup(const char *path, const char *root) {
	struct socket_node *snode;

	socket_endpoints_path = path;
	if (mkdir(socket_endpoints_path, 0777) != 0 && errno != EEXIST) {
		syslog(LOG_ERR, "socket_switch_setup: Unable to create controllers directory '%s': %m", socket_endpoints_path);
		return;
	}

	snode = socket_endpoint_node_create(root, CAPSET_ALL);
	if (snode == NULL) {
		syslog(LOG_ERR, "socket_switch_setup: Unable to create '%s' root endpoint", root);
		return;
	}

	socket_switch_insert(snode);
}

/**
 * Recursively destroy a socket node tree elements.
 * @param element Socket node.
 */
static void
socket_switch_destroy_element(tree_element_t *element) {
	struct socket_node * const snode = element;
	snode->class->destroy(snode);
}

/** Destroy all socket nodes */
void
socket_switch_teardown(void) {
	tree_mutate(&socket_switch.snodes, socket_switch_destroy_element);
#ifdef CONFIG_MEMORY_CLEANUP
	tree_deinit(&socket_switch.snodes);
#endif
}

/** Insert a new socket node */
void
socket_switch_insert(struct socket_node *snode) {
	FD_SET(snode->fd, &socket_switch.activeset);
	tree_insert(&socket_switch.snodes, snode);
}

/** Remove a socket node */
void
socket_switch_remove(struct socket_node *snode) {
	[[maybe_unused]] struct socket_node * const removed = tree_remove(&socket_switch.snodes, snode);

	assert(removed == snode);

	FD_CLR(snode->fd, &socket_switch.activeset);
	snode->class->destroy(snode);
}

/**
 * Prepare the file descriptor sets for _pselect(2)_.
 * @param[out] readfdsp
 * @param[out] writefdsp
 * @param[out] exceptfdsp
 * @returns Maximum file descriptor value, or zero if none in the switch.
 */
int
socket_switch_prepare(fd_set **readfdsp, fd_set **writefdsp, fd_set **exceptfdsp) {
	const struct socket_node *snode;
	int nfds;

	*readfdsp = memcpy(&socket_switch.readset, &socket_switch.activeset, sizeof (**readfdsp));
	*writefdsp = NULL;
	*exceptfdsp = NULL;

	snode = tree_last(&socket_switch.snodes);
	if (snode != NULL) {
		nfds = snode->fd + 1;
	} else {
		nfds = 0;
	}

	return nfds;
}

/**
 * Find a socket node from its file descriptor
 * @param fd A filedescriptor associated with one socket node.
 * @returns Node associated with @p fd.
 */
static inline struct socket_node *
socket_switch_find(int fd) {
	const struct socket_node element = { .fd = fd };

	return tree_find(&socket_switch.snodes, &element);
}

/**
 * Operate at most @p nfds.
 * @param nfds Number of ready file descriptors.
 */
void
socket_switch_operate(int nfds) {

	for (int fd = 0; fd < FD_SETSIZE && nfds != 0; fd++) {
		if (FD_ISSET(fd, &socket_switch.readset)) {
			struct socket_node * const snode = socket_switch_find(fd);

			snode->class->operate(snode);

			nfds--;
		}
	}
}
