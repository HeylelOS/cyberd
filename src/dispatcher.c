#include "dispatcher.h"
#include "log.h"
#include "tree.h"
#include "fde.h"
#include "config.h"

#include <stdlib.h>
#include <strings.h> /* for FD_COPY, ... on Darwin */
#include <sys/socket.h> /* shutdown() */

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

static void
dispatcher_preorder_cleanup(struct tree_node *node) {

	if(node != NULL) {
		struct fd_element *fde = node->element;

		fde_destroy(fde);

		dispatcher_preorder_cleanup(node->left);
		dispatcher_preorder_cleanup(node->right);
	}
}

static void
dispatcher_deinit(void) {

	dispatcher_preorder_cleanup(dispatcher.fds.root);
}

void
dispatcher_init(void) {

	dispatcher.sets = calloc(1, sizeof(dispatcher.sets));

	tree_init(&dispatcher.fds, dispatcher_hash_fd);

	struct fd_element *acceptor
		= fde_create_acceptor(DISPATCHER_IPC_SOCKNAME);

	if(acceptor != NULL) {
		dispatcher_insert(acceptor);

		/* Cleanup needed because we must unlink acceptors */
		atexit(dispatcher_deinit);
	}

}

int
dispatcher_lastfd(void) {
	struct tree_node *current = dispatcher.fds.root;
	int lastfd = -1;

	while(current != NULL) {
		current = current->right;
	}

	if(current != NULL) {
		const struct fd_element *fde = current->element;

		lastfd = fde->fd;
	}

	return lastfd;
}

fd_set *
dispatcher_readset(void) {

	return &dispatcher.sets->readset;
}

static void
dispatcher_handle_acceptor(struct fd_element *fde) {

	log_print("Can read acceptor %d\n", fde->fd);
}

static void
dispatcher_handle_connection(struct fd_element *fde) {

	log_print("Can read connection %d\n", fde->fd);

	shutdown(fde->fd, SHUT_RDWR);
	dispatcher_remove(fde);
}

void
dispatcher_handle(unsigned int fds) {
	fd_set *readset = &dispatcher.sets->readset;

	for(unsigned int fd = 0;
		fd < fds;
		fd += 1) {

		if(FD_ISSET(fd, readset)) {
			struct fd_element *fde = dispatcher_find(fd);

			switch(fde->type) {
			case FD_TYPE_ACCEPTOR:
				dispatcher_handle_acceptor(fde);
				break;
			case FD_TYPE_CONNECTION:
				dispatcher_handle_connection(fde);
				break;
			default:
				break;
			}
		}
	}

	FD_COPY(&dispatcher.sets->activeset, readset);
}

