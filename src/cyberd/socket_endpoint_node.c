/* SPDX-License-Identifier: BSD-3-Clause */
#include "socket_endpoint_node.h"

#include "capabilities.h"
#include "socket_connection_node.h"
#include "socket_switch.h"
#include "socket_node.h"

#include <stdio.h> /* snprintf */
#include <stdlib.h> /* malloc, free */
#include <syslog.h> /* syslog */
#include <unistd.h> /* unlink, close */
#include <fcntl.h> /* fcntl */
#include <sys/socket.h> /* socket, bind, listen, ... */
#include <sys/un.h> /* sockaddr_un */

#define SOCKADDR_UN_MAXLEN (sizeof (((struct sockaddr_un *)0)->sun_path))

/** Socket endpoint */
struct socket_endpoint_node {
	struct socket_node super; /**< Parent socket node */
	capset_t capabilities; /**< Associated capabilities */
};

/** Socket endpoints location in the host filesystem. Initialized by @ref socket_switch_setup. */
const char *socket_endpoints_path;

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
		syslog(LOG_ERR, "socket_endpoint_node_operate: accept: %m");
		return;
	}

	if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) {
		syslog(LOG_ERR, "socket_endpoint_node_operate: fcntl: %m");
		close(fd);
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
	struct socket_endpoint_node * const endpoint = malloc(sizeof (*endpoint));

	if (endpoint == NULL) {
		goto malloc_failure;
	}

	const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0) {
		syslog(LOG_ERR, "socket_endpoint_node_create: socket %s: %m", name);
		goto socket_failure;
	}

	struct sockaddr_un addr = { .sun_family = AF_UNIX };
	const int namelen = snprintf(addr.sun_path, SOCKADDR_UN_MAXLEN, "%s/%s", socket_endpoints_path, name);
	if ((size_t)namelen >= SOCKADDR_UN_MAXLEN) {
		syslog(LOG_ERR, "socket_endpoint_node_create '%s': Invalid name", name);
		goto path_failure;
	}

	if (bind(fd, (const struct sockaddr *)&addr, sizeof (addr)) != 0) {
		syslog(LOG_ERR, "socket_endpoint_node_create bind '%s': %m", name);
		goto bind_failure;
	}

	if (listen(fd, CONFIG_SOCKET_ENDPOINTS_MAX_CONNECTIONS) != 0) {
		syslog(LOG_ERR, "socket_endpoint_node_create listen '%s': %m", name);
		goto listen_failure;
	}

	endpoint->super.class = &socket_endpoint_node_class;
	endpoint->super.fd = fd;
	endpoint->capabilities = capabilities;

	return &endpoint->super;
listen_failure:
bind_failure:
	unlink(addr.sun_path);
path_failure:
	close(fd);
socket_failure:
	free(endpoint);
malloc_failure:
	return NULL;
}
