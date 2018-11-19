#include "fde.h"
#include "log.h"

#include "../config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h> /* to show overflow in fde_create_acceptor */

#define SOCKADDR_UN_MAXLEN sizeof (((struct sockaddr_un *)NULL)->sun_path)

struct fd_element *
fde_create_acceptor(const char *path) {
	struct fd_element *fde = NULL;
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);

	if (fd != -1) {
		struct sockaddr_un addr;
		char * const end = stpncpy(addr.sun_path, path, SOCKADDR_UN_MAXLEN);

		if (end < addr.sun_path + SOCKADDR_UN_MAXLEN) {
			addr.sun_family = AF_LOCAL;

			if (bind(fd, (const struct sockaddr *)&addr, sizeof (addr)) == 0) {
				if (listen(fd, CYBERD_CONNECTIONS_LIMIT) == 0) {
					fde = malloc(sizeof (*fde));

					fde->type = FD_TYPE_ACCEPTOR;
					fde->fd = fd;
				} else {
					log_error("fde_create_acceptor listen");
				}
			} else {
				log_error("fde_create_acceptor bind");
			}
		} else {
			errno = EOVERFLOW;
			log_error("fde_create_acceptor");
		}
	} else {
		log_error("fde_create_acceptor socket");
	}

	return fde;
}

struct fd_element *
fde_create_connection(const struct fd_element *acceptor) {
	struct fd_element *fde = NULL;
	struct sockaddr_un addr;
	socklen_t len;
	int fd = accept(acceptor->fd, (struct sockaddr *)&addr, &len);

	if (fd != -1) {
		fde = malloc(sizeof (*fde));

		fde->type = FD_TYPE_CONNECTION;
		fde->fd = fd;
	} else {
		log_error("fde_create_connection accept");
	}

	return fde;
}

void
fde_destroy(struct fd_element *fde) {

	if (fde->type == FD_TYPE_ACCEPTOR) {
		struct sockaddr_un addr;
		socklen_t len = sizeof (addr);

		if (getsockname(fde->fd,
			(struct sockaddr *)&addr,
			&len) == 0) {
			unlink(addr.sun_path);
		}
	}

	close(fde->fd);

	free(fde);
}

