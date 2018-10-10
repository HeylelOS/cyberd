#include "log.h"
#include "signals.h"
#include "configuration.h"
#include "scheduler/scheduler.h"
#include "networker/networker.h"
#include "spawns/spawns.h"
#include "daemons/daemons.h"

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>

/**
 * The scheduler is a structure meant to planify actions cyberd must do,
 * now or in the future.
 */
struct scheduler scheduler;
/**
 * The networker is a structure meant to handle IPCs and file descriptor
 * related events handling.
 */
struct networker networker;
/**
 * The spawns structure holds every daemon instance running,
 * technically, it's only used in daemon.c and signals.c when
 * forks and a sigchlds happen.
 */
struct spawns spawns;
/**
 * The daemons is a storage structure, it stores the lone allocation of
 * each daemon representation, only pointers are then manipulated elsewhere.
 * It provides function to index easily and access quickly those.
 */
struct daemons daemons;
/**
 * The running boolean determines when the main loop shall end.
 */
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
	spawns_init(&spawns);
	daemons_init(&daemons);
	running = true;
	configuration(CONFIG_LOAD);

	log_print("Entering main loop...\n");
	while (running) {
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

		if (fds > 0) {
			/* Network I/O */

			fds = networker_reading(&networker, fds);
			fds = networker_writing(&networker, fds);
		} else if (fds == 0) {
			/* An event timed out */
			struct scheduler_activity activity;

			scheduler_dequeue(&scheduler, &activity);

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
			/* Error, interrupts not considered */
			log_error("[cyberd pselect]");
		}
	}

	daemons_destroy(&daemons);
	spawns_destroy(&spawns);
	networker_destroy(&networker);
	scheduler_destroy(&scheduler);

	log_print("Finished...\n");

	exit(EXIT_SUCCESS);
}

