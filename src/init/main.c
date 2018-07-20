#include "log.h"
#include "init.h"
#include "config.h"
#include "signals.h"

#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>

struct init init;

int
main(int argc,
	char **argv) {
	sigset_t set; /* Authorized signals while in pselect */

	/* Process environnement */
	init_signals(&set);

	/* Process configuration */
	scheduler_init(&init.scheduler);
	networker_init(&init.networker);
	daemons_init(&init.daemons);
	init.running = true;
	configuration(CONFIG_LOAD);

	log_print("Entering main loop...\n");
	while(init.running) {
		int fds;
		fd_set *readfdsp, *writefdsp;
		const struct timespec *timeoutp;

		/* Fetch networker indications */
		fds = networker_lastfd(&init.networker) + 1;
		readfdsp = networker_readset(&init.networker);
		writefdsp = networker_writeset(&init.networker);

		/* Fetch time before next action */
		timeoutp = scheduler_next(&init.scheduler);

		/* Wait for action */
		fds = pselect(fds,
			readfdsp, writefdsp, NULL,
			timeoutp,
			&set);

		if(fds > 0) {
			/* Network I/O */

			fds = networker_reading(&init.networker, fds);
			fds = networker_writing(&init.networker, fds);
		} else if(fds == 0) {
			/* An event timed out */
			struct scheduler_activity activity;

			scheduler_dequeue(&init.scheduler, &activity);

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

	daemons_destroy(&init.daemons);
	networker_destroy(&init.networker);
	scheduler_destroy(&init.scheduler);

	log_print("Finished...\n");

	exit(EXIT_SUCCESS);
}

