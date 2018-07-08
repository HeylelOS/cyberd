#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../daemon.h"
#include <sys/types.h>
#include <sys/time.h>

/**
 * scheduler.h holds the structure
 * which contains actions to be executed for
 * each daemon. Its implementation is a
 * MinHeap used as a priority queue.
 */

enum scheduler_action {
	SCHEDULE_START,
	SCHEDULE_STOP,
	SCHEDULE_RELOAD
};

struct scheduler_activity {
	struct daemon *daemon;
	time_t when;
#define SCHEDULING_NOW	0
	enum scheduler_action action;
};

struct scheduler {
	size_t n, alloc;
	struct scheduler_activity *scheduling;
};

void
scheduler_init(struct scheduler *scheduler);

void
scheduler_destroy(struct scheduler *scheduler);

void
scheduler_schedule(struct scheduler *scheduler,
	const struct scheduler_activity *activity);

const struct timespec *
scheduler_next(struct scheduler *scheduler);

/*
 * The scheduler MUST NOT be empty when
 * dequeue is called
 */
void
scheduler_dequeue(struct scheduler *scheduler,
	struct scheduler_activity *activity);

void
scheduler_empty(struct scheduler *scheduler);

/* SCHEDULER_H */
#endif
