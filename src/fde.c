#include "fde.h"
#include "log.h"
#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKADDR_UN_MAXLEN sizeof (((struct sockaddr_un *)NULL)->sun_path)

struct fd_element *
fde_create_acceptor(const char *path) {
	struct fd_element *fde = NULL;
	int fd = socket(PF_LOCAL, SOCK_STREAM, 0);

	if (fd != -1) {
		struct sockaddr_un addr;
		char * const end = stpncpy(addr.sun_path, path, SOCKADDR_UN_MAXLEN);

		/* Less than or EQUAL because the maximum is
		authorized, even if no NUL char written */
		if (end <= addr.sun_path + SOCKADDR_UN_MAXLEN) {
			addr.sun_len = end - addr.sun_path;
			addr.sun_family = AF_UNIX;

			if (bind(fd, (const struct sockaddr *)&addr, sizeof (addr)) == 0) {
				if (listen(fd, DISPATCHER_CONNECTIONS_LIMIT) == 0) {
					fde = malloc(sizeof (*fde));

					fde->type = FD_TYPE_ACCEPTOR;
					fde->fd = fd;
				} else {
					log_error("[cyberd fde_create_acceptor listen]");
				}
			} else {
				log_error("[cyberd fde_create_acceptor bind]");
			}
		} else {
			log_print("[cyberd fde_create_acceptor]: error: path overflow\n");
		}
	} else {
		log_error("[cyberd fde_create_acceptor socket]");
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
	}

	return fde;
}

void
fde_destroy(struct fd_element *fde) {

	if (fde->type == FD_TYPE_ACCEPTOR) {
		char *path;
		struct sockaddr_un addr;
		socklen_t len = sizeof (addr);

		getsockname(fde->fd,
			(struct sockaddr *)&addr,
			&len);

		/* Knowing we accept SOCKADDR_UN_MAXLEN size paths */
		if (addr.sun_len == SOCKADDR_UN_MAXLEN) {
			path = alloca(addr.sun_len + 1);
			strncpy(path, addr.sun_path, addr.sun_len);
		} else {
			path = addr.sun_path;
			path[addr.sun_len] = '\0';
		}

		unlink(path);
	}

	close(fde->fd);
	free(fde);
}

