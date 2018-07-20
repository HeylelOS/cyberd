#ifndef NETWORKER_H
#define NETWORKER_H

#include "../daemon.h"
#include <sys/select.h>

struct networker {
	fd_set activefds;

	int lastfd;
};

void
networker_init(struct networker *networker);

void
networker_destroy(struct networker *networker);

int
networker_lastfd(struct networker *networker);

fd_set *
networker_readset(struct networker *networker);

fd_set *
networker_writeset(struct networker *networker);

int
networker_reading(struct networker *networker,
	int fds);

int
networker_writing(struct networker *networker,
	int fds);

/* NETWORKER_H */
#endif
