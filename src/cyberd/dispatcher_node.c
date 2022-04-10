#include "dispatcher_node.h"
#include "configuration.h"
#include "config.h"
#include "log.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h> /* NAME_MAX */
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKADDR_UN_MAXLEN sizeof (((struct sockaddr_un *)0)->sun_path)

struct dispatcher_node_ipc {
	struct dispatcher_node super;
	uint64_t whenorperms;
	union {
		struct ipc_name {
			char *value;
			size_t capacity;
			size_t length;
		} name;
		size_t left;
	};
	enum dispatcher_node_ipc_state state;
	uint8_t id;
};

static void
ipc_name_init(struct ipc_name *name) {
	name->capacity = CONFIG_CONNECTIONS_NAME_BUFFER_DEFAULT_CAPACITY;
	name->length = 0;
	name->value = malloc(name->capacity);
}

static void
ipc_name_deinit(struct ipc_name *name) {
	free(name->value);
}

static bool
ipc_name_append(struct ipc_name *name, uint8_t byte) {
	while ((name->length != 0 || (byte != '\0' && byte != '.')) && byte != '/' && name->length < NAME_MAX) {
		if (name->length == name->capacity - 1) {
			size_t newcapacity = name->capacity * 2;
			char *newvalue = realloc(name->value, newcapacity);

			if (newvalue == NULL) {
				break;
			}

			name->capacity = newcapacity;
			name->value = newvalue;
		}

		name->value[name->length] = byte;
		name->length++;

		return true;
	}

	return false;
}

struct dispatcher_node *
dispatcher_node_create_endpoint(const char *name, perms_t perms) {
	struct sockaddr_un addr = { .sun_family = AF_LOCAL };
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	struct dispatcher_node *endpoint;

	if (fd == -1) {
		log_error("dispatcher_node_create_endpoint socket %s: %m", name);
		goto dispatcher_node_create_endpoint_err0;
	}

	if (snprintf(addr.sun_path, SOCKADDR_UN_MAXLEN,
		CONFIG_ENDPOINTS_DIRECTORY"/%s", name) >= SOCKADDR_UN_MAXLEN) {
		errno = EOVERFLOW;
		log_error("dispatcher_node_create_endpoint '%s': %m", name);
		goto dispatcher_node_create_endpoint_err1;
	}

	if (bind(fd, (const struct sockaddr *)&addr, sizeof (addr)) != 0) {
		log_error("dispatcher_node_create_endpoint bind '%s': %m", name);
		goto dispatcher_node_create_endpoint_err1;
	}

	if (listen(fd, CONFIG_ENDPOINTS_CONNECTIONS) != 0) {
		log_error("dispatcher_node_create_endpoint listen '%s': %m", name);
		goto dispatcher_node_create_endpoint_err2;
	}

	if (endpoint = malloc(sizeof (*endpoint)), endpoint == NULL) {
		goto dispatcher_node_create_endpoint_err2;
	}

	endpoint->isendpoint = true;
	endpoint->perms = perms;
	endpoint->fd = fd;

	return endpoint;
dispatcher_node_create_endpoint_err2:
	unlink(addr.sun_path);
dispatcher_node_create_endpoint_err1:
	close(fd);
dispatcher_node_create_endpoint_err0:
	return NULL;
}

struct dispatcher_node *
dispatcher_node_create_ipc(const struct dispatcher_node *endpoint) {
	struct sockaddr_un addr;
	socklen_t len = sizeof (addr);

	int fd = accept(endpoint->fd, (struct sockaddr *)&addr, &len);
	if (fd == -1) {
		log_error("dispatcher_node_create_ipc accept: %m");
		goto dispatcher_node_create_ipc_err0;
	}

	struct dispatcher_node_ipc *ipc = malloc(sizeof (*ipc));
	if (ipc == NULL) {
		goto dispatcher_node_create_ipc_err1;
	}

	ipc->super.isendpoint = false;
	ipc->super.fd = fd;
	ipc->super.perms = endpoint->perms;
	ipc->state = IPC_AWAITING_ID;

	return &ipc->super;
dispatcher_node_create_ipc_err1:
	close(fd);
dispatcher_node_create_ipc_err0:
	return NULL;
}

void
dispatcher_node_destroy(struct dispatcher_node *dispatched) {
	if (dispatched->isendpoint) {
		struct sockaddr_un addr;
		socklen_t len = sizeof (addr);

		if (getsockname(dispatched->fd, (struct sockaddr *)&addr, &len) == 0) {
			unlink(addr.sun_path);
		}
	} else {
		struct dispatcher_node_ipc *ipc = (struct dispatcher_node_ipc *)dispatched;

		switch (ipc->state) {
		case IPC_AWAITING_NAME:
		case IPC_CREATE_ENDPOINT:
		case IPC_DAEMON:
			ipc_name_deinit(&ipc->name);
			break;
		default:
			break;
		}
	}

	close(dispatched->fd);
	free(dispatched);
}

struct dispatcher_node *
dispatcher_node_ipc_create_endpoint(const struct dispatcher_node *dispatched) {
	const struct dispatcher_node_ipc *ipc = (const struct dispatcher_node_ipc *)dispatched;

	return dispatcher_node_create_endpoint(ipc->name.value, ipc->super.perms & ~ipc->whenorperms);
}

enum dispatcher_node_ipc_state
dispatcher_node_ipc_input(struct dispatcher_node *dispatched, uint8_t byte) {
	struct dispatcher_node_ipc *ipc = (struct dispatcher_node_ipc *)dispatched;

	switch (ipc->state) {
	case IPC_CREATE_ENDPOINT:
		/* fallthrough */
	case IPC_DAEMON:
		ipc_name_deinit(&ipc->name);
		/* fallthrough */
	case IPC_SYSTEM:
		/* fallthrough */
	case IPC_AWAITING_ID:
		ipc->id = byte;
		if (COMMAND_IS_VALID(ipc->id)) {
			ipc->state = IPC_AWAITING_WHEN_OR_PERMS;
			ipc->left = sizeof (ipc->whenorperms);
			ipc->whenorperms = 0;
		} else {
			ipc->state = IPC_DISCARDING;
		}
		break;
	case IPC_AWAITING_WHEN_OR_PERMS:
		ipc->left--;
		ipc->whenorperms |= byte << ipc->left * 8;
		if (ipc->left == 0) {
			ipc->whenorperms = ntohll(ipc->whenorperms);
			if (!COMMAND_IS_SYSTEM(ipc->id)) {
				ipc->state = IPC_AWAITING_NAME;
				ipc_name_init(&ipc->name);
			} else if (PERMS_AUTHORIZES(ipc->super.perms, ipc->id)) {
				ipc->state = IPC_SYSTEM;
			} else {
				ipc->state = IPC_AWAITING_ID;
			}
		}
		break;
	case IPC_AWAITING_NAME:
		if (ipc_name_append(&ipc->name, byte)) {
			if (byte == '\0') {
				if (PERMS_AUTHORIZES(ipc->super.perms, ipc->id)) {
					ipc->state = ipc->id == COMMAND_CREATE_ENDPOINT ?
						IPC_CREATE_ENDPOINT : IPC_DAEMON;
				} else {
					ipc->state = IPC_AWAITING_ID;
					ipc_name_deinit(&ipc->name);
				}
			}
		} else {
			ipc->state = IPC_DISCARDING;
			ipc_name_deinit(&ipc->name);
		}
		break;
	case IPC_DISCARDING:
		if (byte == '\0') {
			ipc->state = IPC_AWAITING_ID;
		}
		break;
	}

	return ipc->state;
}

void
dispatcher_node_ipc_activity(const struct dispatcher_node *dispatched, struct scheduler_activity *activity) {
	const struct dispatcher_node_ipc *ipc = (const struct dispatcher_node_ipc *)dispatched;

	activity->when = ipc->whenorperms;
	activity->action = ipc->id;

	if (COMMAND_IS_DAEMON(ipc->id)) {
		activity->daemon = configuration_daemon_find(ipc->name.value);
	}
}

