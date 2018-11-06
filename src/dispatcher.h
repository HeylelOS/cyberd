#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <sys/select.h>

void
dispatcher_init(void);

int
dispatcher_lastfd(void);

struct fd_set *
dispatcher_readset(void);

struct fd_set *
dispatcher_writeset(void);

void
dispatcher_handle(int fds);

/* DISPATCHER_H */
#endif
