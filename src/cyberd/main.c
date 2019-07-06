#include "configuration.h"
#include "dispatcher.h"
#include "scheduler.h"
#include "spawns.h"
#include "signals.h"
#include "log.h"

#include "config.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> /* sync */
#include <time.h> /* nanosleep */
#include <sys/stat.h> /* umask */
#include <sys/reboot.h> /* reboot */
#include <dirent.h>
#include <errno.h>
#include <err.h>

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

#ifdef RB_HALT_SYSTEM
#define RB_HALT RB_HALT_SYSTEM
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
	ENDING_POWEROFF,
	ENDING_HALT,
	ENDING_REBOOT
} ending;

/**
 * Initialize umask, check/purge init environnement
 */
static void
begin(void) {

	umask(CONFIG_DEFAULT_UMASK);

#ifndef CONFIG_DEBUG
	if(getpid() != 1) {
		errx(EXIT_FAILURE, "Must be run as pid 1");
	}

	if(setuid(0) != 0) {
		errx(EXIT_FAILURE, "Must be run as root");
	}
#endif
}

/**
 * Ending init, lot of cleanup, daemons timeout is 5 seconds,
 * Note some structures (scheduler) are not freed by default, because
 * they do not hold sensible system ressources, such as locks, files..
 */
static void NORETURN
end(void) {
	struct timespec req = {
		.tv_sec = 5,
		.tv_nsec = 0
	}, rem;

	log_print("Ending...");

	/* Notify spawns they should stop */
	spawns_stop();
	/* Destroying dispatcher, to unlink acceptors */
	dispatcher_deinit();
	/* Modifying the signal mask so we accept SIGCHLD now */
	signals_stopping();

	/* While there are spawns, and we didn't time out,
	we wait spawns for 5 seconds */
	while(!spawns_empty()
		&& nanosleep(&req, &rem) == -1
		&& errno == EINTR) {
		req = rem;
	}

	/* If some children didn't exit, we re-block SIGCHLD
	and SIGKILL/wait everyone else (the wait is meant to log them) */
	if(!spawns_empty()) {
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
	note: Standard specifies it may return before all syncs done */
	sync();

	/* Nothing standard in the following part */
	int howto;
	switch(ending) {
#ifdef RB_POWER_OFF
	case ENDING_POWEROFF:
		howto = RB_POWER_OFF;
		break;
#else
#warning "Unsupported poweroff operation will not be available"
#endif
#ifdef RB_HALT
	case ENDING_HALT:
		howto = RB_HALT;
		break;
#else
#warning "Unsupported halt operation will not be available"
#endif
	default: /* ENDING_REBOOT */
		howto = RB_AUTOBOOT;
		break;
	}

	reboot(howto);
	/* We reached an error */
	exit(EXIT_FAILURE);
}

/**
 * System is supended if supported
 */
static void
suspend(void) {

#ifdef RB_SW_SUSPEND
	if(reboot(RB_SW_SUSPEND) == -1) {
		log_error("reboot(RB_SW_SUSPEND)");
	}
#else
#warning "Unsupported suspend operation will not be available"
	log_print("Suspend not available on this operating system");
#endif
}

int
main(int argc,
	char **argv) {
	/* Environnement initialization, order matters */
	begin();
	log_init(*argv);
	signals_init();
	spawns_init();
	scheduler_init();
	dispatcher_init();
	configuration_init();
	running = true;

	log_print("Entering main loop...");
	while(running) {
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
			signals_sigset());

		if(fds > 0) {
			/* Fdset I/O */

			dispatcher_handle(fds);
		} else if(fds == 0) {
			/* An event timed out */
			struct scheduler_activity activity;

			scheduler_dequeue(&activity);

			switch(activity.action) {
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
				if(COMMAND_IS_SYSTEM(activity.action)) {
					if(activity.action == SCHEDULE_SYSTEM_SUSPEND) {
						suspend();
					} else {
						ending = activity.action - SCHEDULE_SYSTEM_POWEROFF;
						running = false;
					}
				} else {
					log_print("Unknown scheduled action received");
				} break;
			}
		} else if(errno != EINTR) {
			/* Error, signal not considered */
			log_error("pselect");
		}
	}

	/* Clean up, and shutdown/reboot */
	end();
}

