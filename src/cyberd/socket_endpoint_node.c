/* SPDX-License-Identifier: BSD-3-Clause */
#include "socket_endpoint_node.h"

#include "capabilities.h"
#include "socket_connection_node.h"
#include "socket_switch.h"
#include "socket_node.h"
#include "log.h"

#include "config.h"

#include <stdio.h> /* snprintf */
#include <stdlib.h> /* malloc, free */
#include <unistd.h> /* unlink, close */
#include <sys/socket.h> /* socket, bind, listen, ... */
#include <sys/un.h> /* sockaddr_un */

#define SOCKADDR_UN_MAXLEN (sizeof (((struct sockaddr_un *)0)->sun_path))

/** Socket endpoint */
struct socket_endpoint_node {
	struct socket_node super; /**< Parent socket node */
	capset_t capabilities; /**< Associated capabilities */
};

/** Socket endpoint operate, accept incoming connection and record it in socket switch. */
static void
socket_endpoint_node_operate(struct socket_node *snode) {
	const struct socket_endpoint_node * const endpoint = (const struct socket_endpoint_node *)snode;
	struct sockaddr_un addr;
	socklen_t len;
	int fd;

	len = sizeof (addr);
	fd = accept(endpoint->super.fd, (struct sockaddr *)&addr, &len);
	if (fd < 0) {
		log_error("socket_endpoint_node_operate: accept: %m");
		return;
	}

	snode = socket_connection_node_create(fd, endpoint->capabilities);
	if (snode == NULL) {
		close(fd);
		return;
	}

	socket_switch_insert(snode);
}

/** Socket endpoint destroy, unlink endpoint in filesystem and close/free resources. */
static void
socket_endpoint_node_destroy(struct socket_node *snode) {
	struct sockaddr_un addr;
	socklen_t len;

	len = sizeof (addr);
	if (getsockname(snode->fd, (struct sockaddr *)&addr, &len) == 0) {
		unlink(addr.sun_path);
	}

	close(snode->fd);
	free(snode);
}

/** Create a new endpoint. Like creating any network socket. */
struct socket_node *
socket_endpoint_node_create(const char *name, capset_t capabilities) {
	static const struct socket_node_class socket_endpoint_node_class = {
		.operate = socket_endpoint_node_operate,
		.destroy = socket_endpoint_node_destroy,
	};
	const int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	struct sockaddr_un addr = { .sun_family = AF_LOCAL };
	struct socket_endpoint_node *endpoint;

	if (fd < 0) {
		log_error("socket_endpoint_node_create: socket %s: %m", name);
		goto socket_endpoint_node_create_err0;
	}

	const int namelen = snprintf(addr.sun_path, SOCKADDR_UN_MAXLEN, CONFIG_ENDPOINTS_DIRECTORY"/%s", name);
	if (namelen < 0 || namelen >= SOCKADDR_UN_MAXLEN) {
		log_error("socket_endpoint_node_create '%s': Invalid name", name);
		goto socket_endpoint_node_create_err1;
	}

	if (bind(fd, (const struct sockaddr *)&addr, sizeof (addr)) != 0) {
		log_error("socket_endpoint_node_create bind '%s': %m", name);
		goto socket_endpoint_node_create_err1;
	}

	if (listen(fd, CONFIG_ENDPOINTS_CONNECTIONS) != 0) {
		log_error("socket_endpoint_node_create listen '%s': %m", name);
		goto socket_endpoint_node_create_err2;
	}

	if (endpoint = malloc(sizeof (*endpoint)), endpoint == NULL) {
		goto socket_endpoint_node_create_err2;
	}

	endpoint->super.class = &socket_endpoint_node_class;
	endpoint->super.fd = fd;
	endpoint->capabilities = capabilities;

	return &endpoint->super;
socket_endpoint_node_create_err2:
	unlink(addr.sun_path);
socket_endpoint_node_create_err1:
	close(fd);
socket_endpoint_node_create_err0:
	return NULL;
}
