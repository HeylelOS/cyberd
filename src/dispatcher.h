#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <sys/select.h>

#define dispatcher_writeset() NULL
#define dispatcher_errorset() NULL

void
dispatcher_init(void);

/**
 * dispatcher_deinit required because we must unlink acceptors
 */
void
dispatcher_deinit(void);

/**
 * Returns the biggest file descriptor dispatched
 */
int
dispatcher_lastfd(void);

/**
 * Prepare internal readset (copies activeset into it)
 * and returns it
 */
fd_set *
dispatcher_readset(void);

/**
 * Handle fds number of file descriptor, when
 * the dispatcher fd_sets were previously
 * (p)select()'ed
 */
void
dispatcher_handle(unsigned int fds);

/* DISPATCHER_H */
#endif
