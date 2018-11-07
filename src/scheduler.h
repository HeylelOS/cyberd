#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "daemon.h"
#include <sys/types.h>
#include <sys/time.h>

/**
 * scheduler.h holds the structure
 * which contains actions to be executed
 * now or in the future for a daemon.
 * Its implementation is a MinHeap
 * used as a priority queue.
 * Notes:
 * - scheduler_init is useless,
 *   because global vars are zeroe'd but
 *   still here for code clarity.
 * - scheduler_destroy not available because
 *   the kernel cleans up memory better
 *   than userspace.
 */

enum scheduler_action {
	SCHEDULE_START,
	SCHEDULE_STOP,
	SCHEDULE_RELOAD
};

struct scheduler_activity {
	struct daemon *daemon;
	time_t when;
#define SCHEDULING_ASAP	0
	enum scheduler_action action;
};

void
scheduler_init(void);

const struct timespec *
scheduler_next(void);

void
scheduler_empty(void);

void
scheduler_schedule(const struct scheduler_activity *activity);

/*
 * The scheduler MUST NOT be empty when
 * dequeue is called
 */
void
scheduler_dequeue(struct scheduler_activity *activity);

/* SCHEDULER_H */
#endif