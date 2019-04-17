#include "dispatcher.h"
#include "log.h"
#include "tree.h"
#include "fde.h"

#include "configuration.h"
#include "scheduler.h"
#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* memcpy, memset, strncpy */
#include <sys/param.h> /* MAXPATHLEN */

static struct {
	struct {
		fd_set activeset;
		fd_set readset;
	} *sets;

	struct tree fds;
} dispatcher;

static hash_t
dispatcher_hash_fd(const tree_element_t *element) {
	const struct fde *fde = element;

	return fde->fd;
}

static void
dispatcher_insert(struct fde *fde) {
	struct tree_node *node
		= tree_node_create(fde);

	FD_SET(fde->fd, &dispatcher.sets->activeset);
	tree_insert(&dispatcher.fds, node);
}

static void
dispatcher_remove(struct fde *fde) {
	struct tree_node *node
		= tree_remove(&dispatcher.fds, fde->fd);

	FD_CLR(fde->fd, &dispatcher.sets->activeset);
	tree_node_destroy(node);
}

static inline struct fde *
dispatcher_find(int fd) {

	return tree_find(&dispatcher.fds, fd);
}

void
dispatcher_init(void) {

	dispatcher.sets = malloc(sizeof(*dispatcher.sets));
	memset(&dispatcher.sets->activeset, 0, sizeof(fd_set));
	memset(&dispatcher.sets->readset, 0, sizeof(fd_set));

	tree_init(&dispatcher.fds, dispatcher_hash_fd);

	struct fde *acceptor
		= fde_create_acceptor(CONFIG_INITCTL_PATH,
			PERMS_CREATE_CONTROLLER | 
			PERMS_DAEMON_ALL |
			PERMS_SYSTEM_ALL);

	if(acceptor != NULL) {
		dispatcher_insert(acceptor);
	} else {
		log_print("dispatcher_init: Unable to create '%s' acceptor\n",
			CONFIG_INITCTL_PATH);
	}
}

static void
dispatcher_preorder_cleanup(struct tree_node *node) {

	if(node != NULL) {
		struct fde *fde = node->element;

		fde_destroy(fde);

		dispatcher_preorder_cleanup(node->left);
		dispatcher_preorder_cleanup(node->right);
	}
}

void
dispatcher_deinit(void) {

	dispatcher_preorder_cleanup(dispatcher.fds.root);
#ifdef CONFIG_FULL_CLEANUP
	tree_deinit(&dispatcher.fds);
	free(dispatcher.sets);
#endif
}

int
dispatcher_lastfd(void) {
	struct tree_node *current = dispatcher.fds.root;
	int lastfd = -1;

	if(current != NULL) {
		while(current->right != NULL) {
			current = current->right;
		}

		const struct fde *fde = current->element;
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
dispatcher_handle_acceptor(struct fde *acceptor) {
	struct fde *controller
		= fde_create_controller(acceptor);

	if(controller != NULL) {
		dispatcher_insert(controller);
	}
}

static void
dispatcher_handle_controller(struct fde *controller) {
	char buffer[CONFIG_READ_BUFFER_SIZE];
	ssize_t readval;

	if((readval = read(controller->fd, buffer, sizeof(buffer))) > 0) {
		const char *current = buffer;
		const char * const end = current + readval;

		while(current < end) {
			struct control *control = controller->control;
			if(control_update(control, *current)
				&& COMMAND_IS_VALID(control->command)
				&& ((controller->perms | (1 << control->command)) == controller->perms)) {
				if(control->command == COMMAND_CREATE_CONTROLLER) {
					/* We can safely assume sizeof(CONFIG_CONTROLLERS_DIRECTORY) < MAXPATHLEN */
					char path[MAXPATHLEN];
					char *name = stpncpy(path, CONFIG_CONTROLLERS_DIRECTORY,
						sizeof(CONFIG_CONTROLLERS_DIRECTORY));
					*name = '/';
					strncpy((name += 1), control->cctl.name.value,
						MAXPATHLEN - sizeof(CONFIG_CONTROLLERS_DIRECTORY) - 1);
					path[MAXPATHLEN - 1] = '\0';
					struct fde *acceptor
						= fde_create_acceptor(path,
							controller->perms
							& ~control->cctl.permsmask);

					if(acceptor != NULL) {
						dispatcher_insert(acceptor);
					} else {
						log_print("dispatcher_handle_controller: Unable to create '%s' acceptor\n",
							control->cctl.name.value);
					}
				} else {
					struct scheduler_activity activity = {
						.when = control->planified.when,
						.action = (int)control->command
					};

					if(COMMAND_IS_SYSTEM(control->command)
						|| (COMMAND_IS_DAEMON(control->command) 
							&& (activity.daemon = configuration_daemon_find(control->planified.daemonhash)) != NULL)) {
						scheduler_schedule(&activity);
					} else {
						log_print("dispatcher_handle_controller: Unable to find daemon\n");
					}
				}
			}

			current += 1;
		}
	} else if(readval == 0) {
		dispatcher_remove(controller);
		fde_destroy(controller);
	}
}

void
dispatcher_handle(unsigned int fds) {
	fd_set *readset = &dispatcher.sets->readset;

	for(unsigned int fd = 0;
		fds != 0 && fd < FD_SETSIZE;
		fd += 1) {

		if(FD_ISSET(fd, readset)) {
			struct fde *fde = dispatcher_find(fd);
			fds -= 1;

			if(fde->type == FDE_TYPE_ACCEPTOR) {
				dispatcher_handle_acceptor(fde);
			} else {
				dispatcher_handle_controller(fde);
			}
		}
	}
}

