#ifndef SPAWNS_H
#define SPAWNS_H

#include "daemon.h"

#include <stdbool.h>

/*
 * spawns.h holds the structure which records every
 * spawn of every daemons. Only daemon.c accesses it for safety
 */

/**
 * Initializes internal data structures
 */
void
spawns_init(void);

/**
 * Tries to daemon_stop() everyone
 */
void
spawns_stop(void);

/**
 * Are there any spawns left?
 * @return Whether there are still daemons in there
 */
bool
spawns_empty(void);

/**
 * daemon_end() everyone left
 */
void
spawns_end(void);

/**
 * Record a new running daemon
 * @param daemon New daemon to record as a spawn
 */
void
spawns_record(struct daemon *daemon);

/**
 * Retrieve a daemon by pid, used in the SIGCHLD handler
 * @param pid PID of the daemon to find
 * @return NULL if no daemon found, the daemon else
 */
struct daemon *
spawns_retrieve(pid_t pid);

/* SPAWNS_H */
#endif
