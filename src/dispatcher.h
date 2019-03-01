#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <sys/select.h>

#define dispatcher_writeset() NULL
#define dispatcher_errorset() NULL

/**
 * Initializes dispatcher structures and main initctl socket
 */
void
dispatcher_init(void);

/**
 * Required because we must unlink socket's acceptors
 */
void
dispatcher_deinit(void);

/**
 * @return The biggest file descriptor dispatched
 */
int
dispatcher_lastfd(void);

/**
 * @return Internal prepared readset (copies activeset into it)
 */
fd_set *
dispatcher_readset(void);

/**
 * @param fds Number of file descriptors to handle, when
 * the dispatcher fd_sets' were just previously (p)select()'ed
 */
void
dispatcher_handle(unsigned int fds);

/* DISPATCHER_H */
#endif
