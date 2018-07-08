#ifndef INIT_H
#define INIT_H

#include "daemons/daemons.h"
#include "scheduler/scheduler.h"

#include <stdbool.h>

extern struct init {
	struct scheduler scheduler;
	struct daemons daemons;

	bool running;
} init;

/* INIT_H */
#endif
