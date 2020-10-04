#include "dispatcher.h"
#include "dispatcher_node.h"
#include "tree.h"
#include "log.h"

#include "configuration.h"
#include "config.h"

#include <unistd.h>
#include <string.h> /* memcpy */
#include <sys/stat.h>
#include <errno.h>

static int
dispatcher_compare_function(const tree_element_t *lhs, const tree_element_t *rhs);

static hash_t
dispatcher_hash_function(const tree_element_t *element);

static struct {
	struct tree nodes;

	fd_set activeset;
	fd_set readset;
} dispatcher;

static const struct tree_class dispatcher_tree_class = {
	.compare_function = dispatcher_compare_function,
	.hash_function = dispatcher_hash_function,
};

static int
dispatcher_compare_function(const tree_element_t *lhs, const tree_element_t *rhs) {
	return (char *)rhs - (char *)lhs;
}

static hash_t
dispatcher_hash_function(const tree_element_t *element) {
	const struct dispatcher_node *dispatched = element;

	return dispatched->fd;
}

static bool
dispatcher_insert(struct dispatcher_node *dispatched) {
	struct tree_node *node = tree_node_create(dispatched);

	if (node != NULL) {
		FD_SET(dispatched->fd, &dispatcher.activeset);
		tree_insert(&dispatcher.nodes, node);

		return true;
	} else {
		return false;
	}
}

static void
dispatcher_remove(struct dispatcher_node *dispatched) {
	struct tree_node *node = tree_remove_by_hash(&dispatcher.nodes, dispatched->fd);

	FD_CLR(dispatched->fd, &dispatcher.activeset);
	tree_node_destroy(node);
}

static inline struct dispatcher_node *
dispatcher_find(int fd) {
	return tree_find_by_hash(&dispatcher.nodes, fd);
}

void
dispatcher_init(void) {
	tree_init(&dispatcher.nodes, &dispatcher_tree_class);

	if (mkdir(CONFIG_ENDPOINTS_DIRECTORY, S_IRWXU | S_IRWXG | S_IRWXO) == 0
		|| errno == EEXIST) { /* If it already exists, we postpone the error if its not a directory */
		struct dispatcher_node *endpoint = dispatcher_node_create_endpoint(CONFIG_ENDPOINT_ROOT, PERMS_ALL);

		if (endpoint != NULL) {
			if (!dispatcher_insert(endpoint)) {
				dispatcher_node_destroy(endpoint);
			}
		} else {
			log_error("dispatcher_init: Unable to create '"CONFIG_ENDPOINT_ROOT"' endpoint");
		}
	} else {
		log_error("dispatcher_init: Unable to create controllers directory '"CONFIG_ENDPOINTS_DIRECTORY"': %m");
	}
}

static void
dispatcher_preorder_cleanup(struct tree_node *node) {
	if (node != NULL) {
		struct dispatcher_node *dispatched = node->element;

		dispatcher_node_destroy(dispatched);

		dispatcher_preorder_cleanup(node->left);
		dispatcher_preorder_cleanup(node->right);
	}
}

void
dispatcher_deinit(void) {
	dispatcher_preorder_cleanup(dispatcher.nodes.root);
#ifdef CONFIG_FULL_CLEANUP
	tree_deinit(&dispatcher.nodes);
#endif
}

int
dispatcher_lastfd(void) {
	struct tree_node *current = dispatcher.nodes.root;
	int lastfd = -1;

	if (current != NULL) {
		while (current->right != NULL) {
			current = current->right;
		}

		const struct dispatcher_node *dispatched = current->element;
		lastfd = dispatched->fd;
	}

	return lastfd;
}

fd_set *
dispatcher_readset(void) {
	return memcpy(&dispatcher.readset, &dispatcher.activeset, sizeof (fd_set));
}

static void
dispatcher_handle_ipc(struct dispatcher_node *ipc) {
	char buffer[CONFIG_READ_BUFFER_SIZE];
	ssize_t readval = read(ipc->fd, buffer, sizeof (buffer));

	if (readval > 0) {
		const char *current = buffer, * const end = buffer + readval;

		while (current != end) {
			switch (dispatcher_node_ipc_input(ipc, *current)) {
			case IPC_CREATE_ENDPOINT: {
				struct dispatcher_node *endpoint = dispatcher_node_ipc_create_endpoint(ipc);

				if (endpoint != NULL) {
					if (!dispatcher_insert(endpoint)) {
						dispatcher_node_destroy(endpoint);
					}
				}
			} break;
			case IPC_DAEMON:
			case IPC_SYSTEM: {
				struct scheduler_activity activity;
				dispatcher_node_ipc_activity(ipc, &activity);
				scheduler_schedule(&activity);
			} break;
			default: /* dismiss */
				break;
			}

			current++;
		}
	} else {
		if (readval != 0) {
			log_error("dispatcher_handle: read: %m");
		}

		dispatcher_remove(ipc);
		dispatcher_node_destroy(ipc);
	}
}

void
dispatcher_handle(unsigned int fds) {
	const fd_set * const readset = &dispatcher.readset;

	for (int fd = 0; fds != 0 && fd < FD_SETSIZE; fd++) {
		if (FD_ISSET(fd, readset)) {
			struct dispatcher_node *dispatched = dispatcher_find(fd);

			if (dispatched->isendpoint) {
				struct dispatcher_node *ipc = dispatcher_node_create_ipc(dispatched);

				if (ipc != NULL && !dispatcher_insert(ipc)) {
					dispatcher_node_destroy(ipc);
				}
			} else {
				dispatcher_handle_ipc(dispatched);
			}

			fds--;
		}
	}
}

