/* SPDX-License-Identifier: BSD-3-Clause */
#include "socket_node.h"

/**
 * Socket switch tree comparison function.
 * @param lhs Left hand side operand.
 * @param rhs Right hand side operand.
 * @returns The comparison between @p lhs and @p rhs fds.
 */
int
socket_nodes_compare(const tree_element_t *lhs, const tree_element_t *rhs) {
	const struct socket_node * const lsnode = lhs, * const rsnode = rhs;

	return lsnode->fd - rsnode->fd;
}
