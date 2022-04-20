/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SOCKET_CONNECTION_NODE_H
#define SOCKET_CONNECTION_NODE_H

#include "capabilities.h"

struct socket_node *
socket_connection_node_create(int fd, capset_t capabilities);

/* SOCKET_CONNECTION_NODE_H */
#endif
