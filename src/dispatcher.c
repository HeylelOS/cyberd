#include "dispatcher.h"
#include "log.h"
#include "tree.h"
#include "fde.h"

#include "scheduler.h"
#include "../config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* shutdown */

static struct {
	struct {
		fd_set activeset;
		fd_set readset;
	} *sets;

	struct tree fds;
} dispatcher;

static hash_t
dispatcher_hash_fd(const tree_element_t *element) {
	const struct fd_element *fde = element;

	return fde->fd;
}

static void
dispatcher_insert(struct fd_element *fde) {
	struct tree_node *node
		= tree_node_create(fde);

	FD_SET(fde->fd, &dispatcher.sets->activeset);
	tree_insert(&dispatcher.fds, node);
}

static void
dispatcher_remove(struct fd_element *fde) {
	struct tree_node *node
		= tree_remove(&dispatcher.fds, fde->fd);

	FD_CLR(fde->fd, &dispatcher.sets->activeset);
	tree_node_destroy(node);
}

static inline struct fd_element *
dispatcher_find(int fd) {

	return tree_find(&dispatcher.fds, fd);
}

void
dispatcher_init(void) {

	dispatcher.sets = malloc(sizeof(*dispatcher.sets));
	memset(&dispatcher.sets->activeset, 0, sizeof(fd_set));
	memset(&dispatcher.sets->readset, 0, sizeof(fd_set));

	tree_init(&dispatcher.fds, dispatcher_hash_fd);

	struct fd_element *acceptor
		= fde_create_acceptor(INITCTL_PATH,
			FDE_PERM_CREATE_CONTROL | 
			FDE_PERM_DAEMON_ALL |
			FDE_PERM_SYSTEM_ALL);

	if(acceptor != NULL) {
		dispatcher_insert(acceptor);
	} else {
		log_print("dispatcher_init: Unable to create '%s' acceptor\n",
			INITCTL_PATH);
	}
}

static void
dispatcher_preorder_cleanup(struct tree_node *node) {

	if(node != NULL) {
		struct fd_element *fde = node->element;

		fde_destroy(fde);

		dispatcher_preorder_cleanup(node->left);
		dispatcher_preorder_cleanup(node->right);
	}
}

void
dispatcher_deinit(void) {

	dispatcher_preorder_cleanup(dispatcher.fds.root);
}

int
dispatcher_lastfd(void) {
	struct tree_node *current = dispatcher.fds.root;
	int lastfd = -1;

	if(current != NULL) {
		while(current->right != NULL) {
			current = current->right;
		}

		const struct fd_element *fde = current->element;
		lastfd = fde->fd;
	}

	return lastfd;
}

fd_set *
dispatcher_readset(void) {

	return memcpy(&dispatcher.sets->readset,
		&dispatcher.sets->activeset,
		sizeof(fd_set));
}

static void
dispatcher_handle_acceptor(struct fd_element *acceptor) {
	struct fd_element *connection
		= fde_create_connection(acceptor);

	if(connection != NULL) {
		dispatcher_insert(connection);
	}
}

static void
dispatcher_handle_connection(struct fd_element *connection) {
	char buffer[512];
	ssize_t readval;

	if((readval = read(connection->fd, buffer, sizeof(buffer))) > 0) {
		const char *current = buffer;
		const char * const end = current + readval;

		while(current < end) {
			if(fde_connection_control(connection, *current)
				&& connection->control.command != COMMAND_UNDEFINED) {
				if(connection->control.command != COMMAND_CREATE_CONTROL) {
				} else {
					struct scheduler_activity activity = {
						.daemon = NULL,
						.when = connection->control.when,
						.action = connection->control.command
					};

					scheduler_schedule(&activity);
				}
			}

			current += 1;
		}
	} else if(readval == 0) {
		dispatcher_remove(connection);
		fde_destroy(connection);
	}
}

void
dispatcher_handle(unsigned int fds) {
	fd_set *readset = &dispatcher.sets->readset;

	for(unsigned int fd = 0;
		fds != 0 && fd < FD_SETSIZE;
		fd += 1) {

		if(FD_ISSET(fd, readset)) {
			struct fd_element *fde = dispatcher_find(fd);
			fds -= 1;

			if((fde->perms & FDE_PERM_ACCEPTOR) != 0) {
				dispatcher_handle_acceptor(fde);
			} else {
				dispatcher_handle_connection(fde);
			}
		}
	}
}

