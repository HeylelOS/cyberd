#ifndef SPAWNS_H
#define SPAWNS_H

#include "daemon.h"

#include <stdbool.h>

/**
 * spawns.h holds the structure which records every
 * spawn of every daemons.
 */

void
spawns_init(void);

void
spawns_stop(void);

bool
spawns_empty(void);

void
spawns_end(void);

void
spawns_record(struct daemon *daemon);

struct daemon *
spawns_retrieve(pid_t pid);

/* SPAWNS_H */
#endif
