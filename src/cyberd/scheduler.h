#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "commands.h"
#include "daemon.h"

#include "config.h"

#include <sys/types.h>
#include <sys/time.h>

/*
 * scheduler.h holds the structure
 * which contains actions to be executed
 * now or in the future for a daemon.
 * Its implementation is a MinHeap
 * used as a priority queue.
 * Notes:
 * - scheduler_init is useless,
 *   because global vars are zeroe'd but
 *   still here for code clarity.
 * - scheduler_deinit not available
 *   by default because the kernel cleans
 *   up memory better than userspace.
 */

/**
 * Which command to execute, see commands.h
 */
enum scheduler_action {
	SCHEDULE_DAEMON_START = COMMAND_DAEMON_START,
	SCHEDULE_DAEMON_STOP = COMMAND_DAEMON_STOP,
	SCHEDULE_DAEMON_RELOAD = COMMAND_DAEMON_RELOAD,
	SCHEDULE_DAEMON_END = COMMAND_DAEMON_END,
	SCHEDULE_SYSTEM_POWEROFF = COMMAND_SYSTEM_POWEROFF,
	SCHEDULE_SYSTEM_HALT = COMMAND_SYSTEM_HALT,
	SCHEDULE_SYSTEM_REBOOT = COMMAND_SYSTEM_REBOOT,
	SCHEDULE_SYSTEM_SUSPEND = COMMAND_SYSTEM_SUSPEND
};

/**
 * Which action to execute, may contain
 * a daemon pointer, that's why the scheduler is emptied
 * at each reconfiguration of cyberd
 */
struct scheduler_activity {
	struct daemon *daemon;
	time_t when;
	enum scheduler_action action;
};

/**
 * Initializes "needed" data (effectively useless, but here for clarity)
 */
void
scheduler_init(void);

#ifdef CONFIG_FULL_CLEANUP
/**
 * Frees all allocations by the scheduler
 */
void
scheduler_deinit(void);
#endif

/**
 * Time until next action
 * @return Time until next action
 */
const struct timespec *
scheduler_next(void);

/**
 * Empties the scheduler
 */
void
scheduler_empty(void);

/**
 * Schedules an activity
 * @param activity What to schedule
 */
void
scheduler_schedule(const struct scheduler_activity *activity);

/*
 * Releasing next activity in parameters, then removes it from scheduler.
 * The scheduler MUST NOT be empty when dequeue is called
 * @param activity Pointer to a structure filled by the next activity.
 */
void
scheduler_dequeue(struct scheduler_activity *activity);

/* SCHEDULER_H */
#endif
