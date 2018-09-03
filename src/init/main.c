#include "log.h"
#include "signals.h"
#include "configuration.h"
#include "scheduler/scheduler.h"
#include "networker/networker.h"
#include "daemons/daemons.h"

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>

struct scheduler scheduler;
struct networker networker;
struct daemons daemons;
bool running;

int
main(int argc,
	char **argv) {
	sigset_t set; /* Authorized signals while in pselect */

	/* Process environnement */
	init_log();
	init_signals(&set);

	/* Process configuration */
	scheduler_init(&scheduler);
	networker_init(&networker);
	daemons_init(&daemons);
	running = true;
	configuration(CONFIG_LOAD);

	log_print("Entering main loop...\n");
	while(running) {
		int fds;
		fd_set *readfdsp, *writefdsp;
		const struct timespec *timeoutp;

		/* Fetch networker indications */
		fds = networker_lastfd(&networker) + 1;
		readfdsp = networker_readset(&networker);
		writefdsp = networker_writeset(&networker);

		/* Fetch time before next action */
		timeoutp = scheduler_next(&scheduler);

		/* Wait for action */
		fds = pselect(fds,
			readfdsp, writefdsp, NULL,
			timeoutp,
			&set);

		if(fds > 0) {
			/* Network I/O */

			fds = networker_reading(&networker, fds);
			fds = networker_writing(&networker, fds);
		} else if(fds == 0) {
			/* An event timed out */
			struct scheduler_activity activity;

			scheduler_dequeue(&scheduler, &activity);

			switch(activity.action) {
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
		} else if(errno != EINTR) {
			/* Error, interrupts not considered */
			log_error("[cyberd pselect]");
		}
	}

	daemons_destroy(&daemons);
	networker_destroy(&networker);
	scheduler_destroy(&scheduler);

	log_print("Finished...\n");

	exit(EXIT_SUCCESS);
}

