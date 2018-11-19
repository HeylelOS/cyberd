#ifndef DAEMON_H
#define DAEMON_H

#include <sys/types.h>  /* pid_t */

#include "hash.h"
#include "daemon_conf.h"

enum daemon_state {
	DAEMON_RUNNING,     /* Has a pid, spawned */
	DAEMON_STOPPED,     /* Was cleared, no process running */
	DAEMON_STOPPING     /* Sent a signal to shut it, spawned */
};

struct daemon {
	char *name;
	enum daemon_state state;

	/* The following are also keys for fast tree access */
	hash_t namehash;
	pid_t pid;

	struct daemon_conf conf;
};

/**
 * Allocates a new daemon, copying name name
 */
struct daemon *
daemon_create(const char *name);

/**
 * Frees every field of the struct, dereference
 * in spawns if not DAEMON_STOPPED, frees the
 * structure
 */
void
daemon_destroy(struct daemon *daemon);

/**
 * Spawns a daemon if it was DAEMON_STOPPED,
 * if successful, record it into spawns.
 * Else does nothing
 */
void
daemon_start(struct daemon *daemon);

/**
 * Send the termination signal if DAEMON_RUNNING
 * setting it to DAEMON_STOPPING
 */
void
daemon_stop(struct daemon *daemon);

/**
 * Sends the signal for reconfiguration
 * to the daemon
 */
void
daemon_reload(struct daemon *daemon);

/**
 * Force kill of the process if not DAEMON_STOPPED
 */
void
daemon_end(struct daemon *daemon);

/* DAEMON_H */
#endif
