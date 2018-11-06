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

	/* The following are keys for fast tree access */
	hash_t namehash;
	pid_t pid;

	struct daemon_conf conf;
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
