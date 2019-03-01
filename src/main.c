#include "configuration.h"
#include "dispatcher.h"
#include "scheduler.h"
#include "spawns.h"
#include "signals.h"
#include "log.h"

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> /* sync */
#include <time.h> /* nanosleep */
#include <sys/stat.h> /* umask */
#include <errno.h>

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

/**
 * The running boolean determines when the main loop shall end.
 * has visibility in the whole process
 */
bool running;

/**
 * The ending enum specifies what to do when we end.
 * Should we poweroff, halt, reboot?
 */
static enum {
	ENDING_POWEROFF = 0,
	ENDING_HALT = 1,
	ENDING_REBOOT = 2,
	ENDING_SUSPEND = 3
} ending;

/**
 * Quick begin check to ensure we are init and set umask
 */
static void
begin(void) {

	umask(CONFIG_DEFAULT_UMASK);

#ifndef CONFIG_DEBUG
	if (getpid() != 1) {
		fprintf(stderr, "cyberd must be pid 1\n");
		exit(EXIT_FAILURE);
	}

	if (geteuid() != 0) {
		fprintf(stderr, "cyberd must be run as root\n");
		exit(EXIT_FAILURE);
	}
#endif
}

/**
 * Ending init, lot of cleanup, the daemons timeout is 5 seconds,
 * Note some structures (scheduler) are not freed, because
 * they do not hold sensible system ressources, such as locks, files..
 */
static void NORETURN
end(void) {
	struct timespec req = {
		.tv_sec = 5,
		.tv_nsec = 0
	}, rem;

	log_print("Ending...\n");

	/* Notify spawns they should stop */
	spawns_stop();
	/* Destroying dispatcher, to unlink acceptors */
	dispatcher_deinit();
	/* Modifying the signal mask so we accept SIGCHLD now */
	signals_stopping();

	/* While there are spawns, and we didn't time out, we wait spawns for 5 seconds */
	while (!spawns_empty() && nanosleep(&req, &rem) == -1 && errno == EINTR) {
		req = rem;
	}

	/* If some children didn't exit, we re-block SIGCHLD
	and SIGKILL/wait everyone else (the wait is meant to log them) */
	if (!spawns_empty()) {
		signals_ending();
		spawns_end();
	}

#ifdef CONFIG_FULL_CLEANUP
	scheduler_deinit();
	configuration_deinit();
#endif

	/* log ending */
	log_deinit();

	/* Synchronize all filesystems to disk(s),
	note: Standard specifies it may return before all syncs done,
	but the Linux kernel is synchronous */
	sync();

	exit(EXIT_SUCCESS);
}

int
main(int argc,
	const char **argv) {
	/* Environnement initialization, order matters */
	begin();
	log_init();
	signals_init();
	spawns_init();
	scheduler_init();
	dispatcher_init();
	configuration_init();
	running = true;

	log_print("Entering main loop...\n");
	while (running) {
		int fds;
		fd_set *readfdsp, *writefdsp, *errorfdsp;
		const struct timespec *timeoutp;

		/* Fetch dispatcher indications */
		fds = dispatcher_lastfd() + 1;
		readfdsp = dispatcher_readset();
		writefdsp = dispatcher_writeset();
		errorfdsp = dispatcher_errorset();

		/* Fetch time before next action */
		timeoutp = scheduler_next();

		/* Wait for action */
		fds = pselect(fds,
			readfdsp, writefdsp, errorfdsp,
			timeoutp,
			&signals_selmask);

		if (fds > 0) {
			/* Fdset I/O */

			dispatcher_handle(fds);
		} else if (fds == 0) {
			/* An event timed out */
			struct scheduler_activity activity;

			scheduler_dequeue(&activity);

			switch (activity.action) {
			case SCHEDULE_DAEMON_START:
				daemon_start(activity.daemon);
				break;
			case SCHEDULE_DAEMON_STOP:
				daemon_stop(activity.daemon);
				break;
			case SCHEDULE_DAEMON_RELOAD:
				daemon_reload(activity.daemon);
				break;
			case SCHEDULE_DAEMON_END:
				daemon_end(activity.daemon);
				break;
			default:
				if (COMMAND_IS_SYSTEM(activity.action)) {
					ending = activity.action - SCHEDULE_SYSTEM_POWEROFF;
					running = false;
				} else {
					log_print("Unknown scheduled action received\n");
				} break;
			}
		} else if (errno != EINTR) {
			/* Error, signal not considered */
			log_error("pselect");
		}
	}

	/* Clean up, and shutdown/reboot */
	end();
}

