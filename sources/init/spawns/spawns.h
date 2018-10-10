#ifndef SPAWNS_H
#define SPAWNS_H

#include "../daemon.h"
#include <sys/types.h>

struct spawns {
	struct spawn *first;
};

void
spawns_init(struct spawns *spawns);

void
spawns_destroy(struct spawns *spawns);

void
spawns_record(struct spawns *spawns,
	struct daemon *daemon,
	pid_t pid);

struct daemon *
spawns_retrieve(struct spawns *spawns,
	pid_t pid);

/* SPAWNS_H */
#endif
