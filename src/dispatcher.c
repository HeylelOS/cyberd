#include "dispatcher.h"
#include "tree.h"

#include <stdlib.h>

struct fd_element {
	int fd;
};

static struct {
	struct {
		fd_set set;
		fd_set readset;
		fd_set writeset;
	} *sets;

	struct tree fds;
} dispatcher;

static hash_t
dispatcher_hash_fd(const tree_element_t *element) {
	const struct fd_element *fde
		= (const struct fd_element *)element;

	return fde->fd;
}

void
dispatcher_init(void) {

	dispatcher.sets = calloc(1, sizeof(dispatcher.sets));
	tree_init(&dispatcher.fds, dispatcher_hash_fd);
}

int
dispatcher_lastfd(void) {
	struct tree_node *current = dispatcher.fds.root;
	int lastfd = -1;

	while(current != NULL) {
		current = current->right;
	}

	if(current != NULL) {
		const struct fd_element *fde
			= (const struct fd_element *)current->element;

		lastfd = fde->fd;
	}

	return lastfd;
}

struct fd_set *
dispatcher_readset(void) {

	return &dispatcher.sets->readset;
}

struct fd_set *
dispatcher_writeset(void) {

	return &dispatcher.sets->writeset;
}

void
dispatcher_handle(int fds) {
}

