#ifndef SPAWNS_H
#define SPAWNS_H

#include "daemon.h"

/**
 * spawns.h holds the structure which records every
 * spawn of every daemons. For now, it's a forward linked list,
 * which doesn't stop one daemon from having multiple spawns.
 * but we ensure it never happens.
 */

void
spawns_init(void);

void
spawns_record(struct daemon *daemon);

struct daemon *
spawns_retrieve(pid_t pid);

/* SPAWNS_H */
#endif
