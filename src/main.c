#include "configuration.h"
#include "dispatcher.h"
#include "scheduler.h"
#include "spawns.h"
#include "signals.h"
#include "log.h"

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>

/**
 * The running boolean determines when the main loop shall end.
 * has visibility in the whole process
 */
bool running;

int
main(int argc,
	char **argv) {
	/* cyberd environnement */
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
		extern const sigset_t signals_selmask;

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
			case SCHEDULE_START:
				daemon_start(activity.daemon);
				break;
			case SCHEDULE_STOP:
				daemon_stop(activity.daemon);
				break;
			case SCHEDULE_RELOAD:
				daemon_reload(activity.daemon);
				break;
			default:
				log_print("Unknown scheduled action received\n");
				break;
			}
		} else if (errno != EINTR) {
			/* Error, signal not considered */
			log_error("[cyberd pselect]");
		}
	}

	log_print("Finished...\n");

	exit(EXIT_SUCCESS);
}

