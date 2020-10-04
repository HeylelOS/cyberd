#ifndef DISPATCHER_NODE_H
#define DISPATCHER_NODE_H

#include "scheduler.h"
#include "commands.h"
#include "perms.h"

#include <stdbool.h>

enum dispatcher_node_ipc_state {
	IPC_AWAITING_ID,
	IPC_AWAITING_WHEN_OR_PERMS,
	IPC_AWAITING_NAME,
	IPC_DISCARDING,
	IPC_CREATE_ENDPOINT,
	IPC_DAEMON,
	IPC_SYSTEM,
};

struct dispatcher_node {
	bool isendpoint;
	perms_t perms;
	int fd;
};

/* Functions for all types of nodes */

struct dispatcher_node *
dispatcher_node_create_endpoint(const char *name, perms_t perms);

struct dispatcher_node *
dispatcher_node_create_ipc(const struct dispatcher_node *endpoint);

void
dispatcher_node_destroy(struct dispatcher_node *dispatched);

/* Functions only for ipc */

struct dispatcher_node *
dispatcher_node_ipc_create_endpoint(const struct dispatcher_node *dispatched);

enum dispatcher_node_ipc_state
dispatcher_node_ipc_input(struct dispatcher_node *dispatched, uint8_t byte);

void
dispatcher_node_ipc_activity(const struct dispatcher_node *dispatched, struct scheduler_activity *activity);

/* DISPATCHER_NODE_H */
#endif
