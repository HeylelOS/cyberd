#ifndef DAEMON_H
#define DAEMON_H

#include <sys/types.h>  /* pid_t */

#include "hash.h"
#include "daemon_conf.h"

/**
 * State of the daemon, ensures only one spawns for each daemon
 */
enum daemon_state {
	DAEMON_RUNNING,     /**< Has a pid, spawned */
	DAEMON_STOPPED,     /**< Was cleared, no process running */
	DAEMON_STOPPING     /**< Sent a signal to shut it, spawned */
};

/**
 * Certainly of the main piece of cyberd,
 * It represents a daemon and its minimum informations,
 * each lone allocation lies in configuration.c as the configuration loads daemon.
 */
struct daemon {
	char *name; /**< Daemon's name */
	enum daemon_state state; /**< Daemon's running state */

	hash_t namehash; /**< Key for tree index by name (used in configuration.c) */
	pid_t pid; /**< Key for tree index by pid (used in spawns.c) */

	struct daemon_conf conf; /**< Daemon configuration */
};

/**
 * Allocates a new daemon
 * @param name Daemon's identifier, string internally copied
 * @return Newly allocated daemon
 */
struct daemon *
daemon_create(const char *name);

/**
 * Frees every field of the struct, dereference
 * in spawns if not DAEMON_STOPPED, frees the structure
 * @param daemon A previously daemon_create()'d daemon
 */
void
daemon_destroy(struct daemon *daemon);

/**
 * Spawns a daemon if it was DAEMON_STOPPED,
 * if successful, record it into spawns.
 * Else does nothing
 * @param daemon Daemon to start
 */
void
daemon_start(struct daemon *daemon);

/**
 * Send the termination signal if DAEMON_RUNNING
 * setting it to DAEMON_STOPPING
 * @param daemon Daemon to stop
 */
void
daemon_stop(struct daemon *daemon);

/**
 * Sends the signal for reconfiguration
 * to the daemon
 * @param daemon Daemon to reload
 */
void
daemon_reload(struct daemon *daemon);

/**
 * Force kill of the process if not DAEMON_STOPPED
 * @param daemon Daemon to end
 */
void
daemon_end(struct daemon *daemon);

/* DAEMON_H */
#endif
