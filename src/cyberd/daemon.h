/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef DAEMON_H
#define DAEMON_H

#include <sys/types.h> /* pid_t */

#include "daemon_conf.h"

/** State of the daemon, ensures only one spawns for each daemon. */
enum daemon_state {
	DAEMON_RUNNING,  /**< Has a pid, spawned. */
	DAEMON_STOPPED,  /**< Was cleared, no process running. */
	DAEMON_STOPPING, /**< Sent a signal to shut it, spawned. */
};

/**
 * Central piece of cyberd, It represents a daemon and its configuration.
 * each instance is owned by `src/cyberd/configuration.c`.
 */
struct daemon {
	enum daemon_state state; /**< Daemon's running state. */

	char *name; /**< Daemon's name, index for configuration. */
	pid_t pid;  /**< Daemon's pid if state == DAEMON_RUNNING, index for spawns. */

	struct daemon_conf conf; /**< Daemon's configuration. */
};

struct daemon *
daemon_create(const char *name);

void
daemon_destroy(struct daemon *daemon);

void
daemon_start(struct daemon *daemon);

void
daemon_stop(struct daemon *daemon);

void
daemon_reload(struct daemon *daemon);

void
daemon_end(struct daemon *daemon);

/* DAEMON_H */
#endif
