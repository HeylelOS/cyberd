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
fde_create_acceptor(const char *path, fde_perms_t perms) {
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

					fde->perms = perms | FDE_PERM_ACCEPTOR;
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

		fde->perms = acceptor->perms & ~FDE_PERM_ACCEPTOR;
		fde->fd = fd;
		fde->connection.state = FDE_CONNECTION_START;
		fde->connection.name = NULL;
		fde->connection.namecap = 0;
	} else {
		log_error("fde_create_connection accept");
	}

	return fde;
}

bool
fde_connection_control(struct fd_element *fde,
	char c) {
	bool ended = false;

	if (c == '\0') {
		if (fde->control.state == FDE_CONNECTION_VALID) {
			ended = true;
		} else {
			fde->control.state == FDE_CONNECTION_START;
		}
	} else {
		switch (fde->control.state) {
		case FDE_CONNECTION_START:
			fde->control.command = COMMAND_UNDEFINED;
			automaton_command_init(&fde->control.automata.command);
			automaton_command_run(&fde->control.automata.command, c);

			if (fde->control.automata.command.state != AUTOMATON_COMMAND_INVALID) {
				fde->control.state = FDE_CONNECTION_TYPE;
			} else {
				fde->control.state = FDE_CONNECTION_DISCARDING;
			}
			break;
		case FDE_CONNECTION_TYPE: {
				enum automaton_command_state state
					= fde->control.automata.command.state;

				if (c == ' ' && AUTOMATON_COMMAND_STATE_VALID(state)) {
					fde->control.state = FDE_CONNECTION_WHEN;
					fde->control.command = state;
					automaton_time_init(&fde->control.automata.time);
				} else {
					automaton_command_run(&fde->control.automata.command, c);
					state = fde->control.automata.command.state;

					if (state == AUTOMATON_COMMAND_INVALID) {
						fde->control.state = FDE_CONNECTION_DISCARDING;
					} /* else stay in FDE_CONNECTION_TYPE */
				}
			} break;
		case FDE_CONNECTION_WHEN:
			break;
		case FDE_CONNECTION_ARGUMENTS:
			break;
		case FDE_CONNECTION_VALID:
			break;
		default: /* FDE_CONNECTION_DISCARDING */
			break;
		}
	}

	return ended;
}

void
fde_destroy(struct fd_element *fde) {

	if ((fde->perms & FDE_PERM_ACCEPTOR) != 0) {
		struct sockaddr_un addr;
		socklen_t len = sizeof (addr);

		if (getsockname(fde->fd,
			(struct sockaddr *)&addr,
			&len) == 0) {
			unlink(addr.sun_path);
		}
	} else {
		free(fde->connection.name);
	}

	close(fde->fd);

	free(fde);
}

