#include "../log.h"
#include "networker.h"

#include <stdlib.h>

struct networker_node {
	int fd;
	struct networker_node *previous;
	struct networker_node *next;
};

static void
networker_insert(struct networker *networker,
	struct networker_node *node) {

	if(!FD_ISSET(node->fd, &networker->activefds)) {
		FD_SET(node->fd, &networker->activefds);

		if(networker->first != NULL) {
			if(networker->last->fd < node->fd) {

				node->previous = networker->last;
				networker->last = node;
			} else if(node->fd < networker->first->fd) {

				node->next = networker->first;
				networker->first = node;
			} else {
				struct networker_node *current = networker->first->next;

				while(node->fd < current->fd) {
					current = current->next;
				}

				node->previous = current;
				node->next = current->next;
				current->next = node;
			}
		} else { /* Empty networker */
			networker->first = node;
			networker->last = node;
		}
	}
}

void
networker_init(struct networker *networker) {

	FD_ZERO(&networker->activefds);

	networker->first = NULL;
	networker->last = NULL;
}

void
networker_destroy(struct networker *networker) {
}

int
networker_lastfd(struct networker *networker) {
	return networker->last == NULL ?
		-1 : networker->last->fd;
}

fd_set *
networker_readset(struct networker *networker) {

	return NULL;
}

fd_set *
networker_writeset(struct networker *networker) {

	return NULL;
}

int
networker_reading(struct networker *networker,
	int fds) {

	return 0;
}

int
networker_writing(struct networker *networker,
	int fds) {

	return 0;
}

