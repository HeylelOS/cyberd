#include "signals.h"
#include "configuration.h"
#include "spawns.h"
#include "log.h"

#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>

static sigset_t initsigset;

static void
sigterm_handler(int sig) {
	extern bool running;

	running = false;
}

static void
sighup_handler(int sig) {

	log_restart();
	configuration_reload();
}

static void
sigchld_handler(int sig) {
	siginfo_t info;

	if (waitid(P_ALL, 0, &info, WEXITED | WNOHANG) == 0) {
		struct daemon *daemon = spawns_retrieve(info.si_pid);

		switch (info.si_code) {
		case CLD_EXITED:
			if (daemon != NULL) {
				daemon->state = DAEMON_STOPPED;
				log_print("sigchld: '%s' (pid: %d) terminated with exit status %d", daemon->name, info.si_pid, info.si_status);

				if (info.si_status == 0) {
					if (daemon->conf.start.exitsuccess == 1) {
						daemon_start(daemon);
					}
				} else if (daemon->conf.start.exitfailure == 1) {
					daemon_start(daemon);
				}
			} else {
				log_print("sigchld: Orphan %d terminated with exit status %d", info.si_pid, info.si_status);
			}
			break;
		case CLD_KILLED:
			if (daemon != NULL) {
				daemon->state = DAEMON_STOPPED;
				log_print("sigchld: '%s' (pid: %d) killed by signal %d", daemon->name, info.si_pid, info.si_status);

				if (daemon->conf.start.killed == 1) {
					daemon_start(daemon);
				}
			} else {
				log_print("sigchld: Orphan %d killed by signal %d", info.si_pid, info.si_status);
			}
			break;
		case CLD_DUMPED:
			if (daemon != NULL) {
				daemon->state = DAEMON_STOPPED;
				log_print("sigchld: '%s' (pid: %d) dumped core", daemon->name, info.si_pid);

				if (daemon->conf.start.dumped == 1) {
					daemon_start(daemon);
				}
			} else {
				log_print("sigchld: Orphan %d dumped core", info.si_pid);
			}
			break;
		default: /* This one should never happen, but whatever */
			if (daemon != NULL) {
				daemon->state = DAEMON_STOPPED;
				log_print("sigchld: '%s' (pid: %d) exited", daemon->name, info.si_pid);
			} else {
				log_print("sigchld: Orphan %d exited", info.si_pid);
			}
			break;
		}
	} else {
		log_error("sigchld waitid: %m");
	}
}

void
signals_init(void) {
	struct sigaction action;

	/**
	 * NOTE: Linux specifies that any signal for which
	 * init hasn't set a signal handler will not be received,
	 * however this behavior may not be consistent through other implementations.
	 * Blocking all signals is not a solution as a malicious program might fill
	 * our pending signals queue with blocked ones, preventing receptions of useful ones.
	 * The solution might be to SIG_IGN all signals, but that wouldn't prevent SIGKILL
	 * and SIGSTOP, and we have no mean of knowing every system's signals from libc.
	 */

	/* Define blocked signals when not in pselect(2) */
	sigemptyset(&initsigset);
	sigaddset(&initsigset, SIGTERM);
#ifdef CONFIG_DEBUG
	sigaddset(&initsigset, SIGINT);
#endif
	sigaddset(&initsigset, SIGHUP);
	sigaddset(&initsigset, SIGCHLD);
	sigprocmask(SIG_SETMASK, &initsigset, NULL);

	/* Init signal handlers */
	sigfillset(&action.sa_mask);

	/* The following is important, because if for any reason we cannot
	open IPC sockets, we might not be able to stop cyberd properly */
	action.sa_flags = 0;
	action.sa_handler = sigterm_handler;
	sigaction(SIGTERM, &action, NULL);
#ifdef CONFIG_DEBUG
	sigaction(SIGINT, &action, NULL);
#endif

	/* The following can have the SA_RESTART flag because when
	called during pselect(2), they do not modify any main loop's
	variable, except for sighup_handler which empties the scheduler */
	action.sa_flags = SA_RESTART;
	action.sa_handler = sighup_handler;
	sigaction(SIGHUP, &action, NULL);
	action.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &action, NULL);

	/* Define pselect's non blocked signals */
	sigfillset(&initsigset);
	sigdelset(&initsigset, SIGTERM);
#ifdef CONFIG_DEBUG
	sigdelset(&initsigset, SIGINT);
#endif
	sigdelset(&initsigset, SIGHUP);
	sigdelset(&initsigset, SIGCHLD);
}

const sigset_t *
signals_sigset(void) {

	return &initsigset;
}

void
signals_stopping(void) {
	sigset_t unblock;

	sigemptyset(&unblock);
	sigaddset(&unblock, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &unblock, NULL);
}

void
signals_ending(void) {
	sigset_t block;

	sigemptyset(&block);
	sigaddset(&block, SIGCHLD);
	sigprocmask(SIG_BLOCK, &block, NULL);
}

