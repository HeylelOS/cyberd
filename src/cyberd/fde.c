#include "fde.h"
#include "log.h"

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h> /* to show overflow in fde_create_acceptor */

#define SOCKADDR_UN_MAXLEN sizeof (((struct sockaddr_un *)NULL)->sun_path)

struct fde *
fde_create_acceptor(const char *path, perms_t perms) {
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);

	if (fd == -1) {
		log_error("fde_create_acceptor socket");
		goto fde_create_acceptor_err0;
	}

	struct sockaddr_un addr = { .sun_family = AF_LOCAL };
	char * const end = stpncpy(addr.sun_path, path, SOCKADDR_UN_MAXLEN);

	if (end >= addr.sun_path + SOCKADDR_UN_MAXLEN) {
		errno = EOVERFLOW;
		log_error("fde_create_acceptor");
		goto fde_create_acceptor_err1;
	}

	if (bind(fd, (const struct sockaddr *)&addr, sizeof (addr)) != 0) {
		log_error("fde_create_acceptor bind");
		goto fde_create_acceptor_err1;
	}

	if (listen(fd, CONFIG_CONNECTIONS_LIMIT) != 0) {
		log_error("fde_create_acceptor listen");
		goto fde_create_acceptor_err2;
	}

	struct fde *fde = malloc(sizeof (*fde));
	fde->fd = fd;
	fde->type = FDE_TYPE_ACCEPTOR;
	fde->perms = perms;
	fde->control = NULL;

	return fde;
fde_create_acceptor_err2:
	unlink(path);
fde_create_acceptor_err1:
	close(fd);
fde_create_acceptor_err0:
	return NULL;
}

struct fde *
fde_create_controller(const struct fde *acceptor) {
	struct fde *fde = NULL;
	struct sockaddr_un addr;
	socklen_t len;
	int fd = accept(acceptor->fd, (struct sockaddr *)&addr, &len);

	if (fd != -1) {
		fde = malloc(sizeof (*fde));

		fde->fd = fd;
		fde->type = FDE_TYPE_CONTROLLER;
		fde->perms = acceptor->perms;
		fde->control = control_create();
	} else {
		log_error("fde_create_controller accept");
	}

	return fde;
}

void
fde_destroy(struct fde *fde) {

	if (fde->type == FDE_TYPE_ACCEPTOR) {
		struct sockaddr_un addr;
		socklen_t len = sizeof (addr);

		if (getsockname(fde->fd,
			(struct sockaddr *)&addr,
			&len) == 0) {
			unlink(addr.sun_path);
		}
	} else {
		control_destroy(fde->control);
	}

	close(fde->fd);

	free(fde);
}

