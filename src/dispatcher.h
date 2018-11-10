#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <sys/select.h>

#define dispatcher_writeset() NULL
#define dispatcher_errorset() NULL

void
dispatcher_init(void);

int
dispatcher_lastfd(void);

fd_set *
dispatcher_readset(void);

void
dispatcher_handle(unsigned int fds);

/* DISPATCHER_H */
#endif
