#ifndef DAEMON_H
#define DAEMON_H

#include <stdint.h>

enum daemon_state {
	DAEMON_RUNNING,
	DAEMON_STOPPED,
	DAEMON_STOPPING
};

typedef uint64_t hash_t;

struct daemon {
	char *name;
	hash_t hash;
	enum daemon_state state;

	char *file;
	char **arguments;
};

void
daemon_init(struct daemon *daemon,
	const char *name);

void
daemon_destroy(struct daemon *daemon);

hash_t
daemon_hash(const char *name);

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
