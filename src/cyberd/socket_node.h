/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SOCKET_NODE_H
#define SOCKET_NODE_H

#include "tree.h"

struct socket_node;

/** Dynamic dispatch table of a socket node. */
struct socket_node_class {
	void (* const operate)(struct socket_node *); /**< Operation to run when a fd is ready for read. */
	void (* const destroy)(struct socket_node *); /**< Destroy and free a socket node. */
};

/** Socket node. */
struct socket_node {
	const struct socket_node_class *class; /**< Class of the node. */
	int fd; /**< File descriptor of the node. */
};

int
socket_nodes_compare(const tree_element_t *lhs, const tree_element_t *rhs);

/* SOCKET_NODE_H */
#endif
