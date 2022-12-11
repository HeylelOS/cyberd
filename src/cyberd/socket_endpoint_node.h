/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SOCKET_ENDPOINT_NODE_H
#define SOCKET_ENDPOINT_NODE_H

#include "capabilities.h"

extern const char *socket_endpoints_path;

struct socket_node *
socket_endpoint_node_create(const char *name, capset_t capabilities);

/* SOCKET_ENDPOINT_NODE_H */
#endif
