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

	scheduler_init(&init.scheduler);
	daemons_init(&init.daemons);
	init.running = true;

	init_signals(&set);
	configuration(CONFIG_LOAD);

	log_print("Entering main loop...\n");
	while (init.running) {
		const struct timespec *timeoutp;
		int fds;

		timeoutp = scheduler_next(&init.scheduler);
		fds = pselect(0, NULL, NULL, NULL, timeoutp, &set);

		if (fds > 0) {
		} else if (fds == 0) {
			struct scheduler_activity activity;

			scheduler_dequeue(&init.scheduler, &activity);

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
		} else {
			log_error("[cyberd pselect]");
		}
	}

	daemons_destroy(&init.daemons);
	scheduler_destroy(&init.scheduler);

	log_print("Finished\n");

	exit(EXIT_SUCCESS);
}

