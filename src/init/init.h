#ifndef INIT_H
#define INIT_H

#include "scheduler/scheduler.h"
#include "networker/networker.h"
#include "daemons/daemons.h"

#include <stdbool.h>

extern struct init {
	struct scheduler scheduler;
	struct networker networker;
	struct daemons daemons;

	bool running;
} init;

/* INIT_H */
#endif
