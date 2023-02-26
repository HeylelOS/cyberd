/* SPDX-License-Identifier: BSD-3-Clause */
#include "signals.h"

#include "configuration.h"
#include "spawns.h"

#include <stdlib.h> /* abort */
#include <sys/reboot.h> /* reboot, RB_POWER_OFF, ... */
#include <sys/wait.h> /* waitid, ... */
#include <syslog.h> /* syslog */
#include <assert.h> /* static_assert */
#include <errno.h> /* errno */

/**
 * Requested reboot action. Used in `src/cyberd/main.c` to check whether
 * termination is requested. Holds the value to use with _reboot(2)_.
 */
int rebootcmd;

/**
 * SIGTERM. Checks for additional informations from a potential _sigqueue(2)_
 * to determine which _reboot(2)_ action to engage.
 * @param siginfo Signal informations.
 */
static void
sigterm_handler(int, siginfo_t *siginfo, void *) {

	/* Obviously, the @ref main loop is broken
	 * if any one of these supported actions equals zero. */
	static_assert(RB_AUTOBOOT != 0);
	static_assert(RB_HALT_SYSTEM != 0);
	static_assert(RB_POWER_OFF != 0);
	static_assert(RB_SW_SUSPEND != 0);

	if (siginfo->si_code == SI_QUEUE) {
		const int value = siginfo->si_value.sival_int;

		switch (value) {
		case RB_AUTOBOOT:    [[fallthrough]];
		case RB_HALT_SYSTEM: [[fallthrough]];
		case RB_POWER_OFF:   [[fallthrough]];
		case RB_SW_SUSPEND:
			rebootcmd = value;
			break;
		default:
			syslog(LOG_ERR, "sigterm: Invalid reboot magic 0x%.8X", value);
		}
	} else {
		rebootcmd = RB_POWER_OFF;
	}
}

/** SIGHUP handler, reload daemons configurations. */
static void
sighup_handler(int) {
	configuration_reload();
}

/** SIGCHLD handler, child processes reaping. */
static void
sigchld_handler(int) {
	siginfo_t info;

	while (info.si_pid = 0, errno = 0, waitid(P_ALL, 0, &info, WEXITED | WNOHANG) == 0 && info.si_pid != 0) {
		struct daemon * const daemon = spawns_retrieve(info.si_pid);

		switch (info.si_code) {
		case CLD_EXITED:
			if (daemon != NULL) {
				daemon->state = DAEMON_STOPPED;
				syslog(LOG_INFO, "sigchld: '%s' (pid: %d) terminated with exit status %d", daemon->name, info.si_pid, info.si_status);

				if (info.si_status == 0) {
					if (daemon->conf.start.exitsuccess == 1) {
						daemon_start(daemon);
					}
				} else if (daemon->conf.start.exitfailure == 1) {
					daemon_start(daemon);
				}
			} else {
				syslog(LOG_INFO, "sigchld: Orphan %d terminated with exit status %d", info.si_pid, info.si_status);
			}
			break;
		case CLD_KILLED:
			if (daemon != NULL) {
				daemon->state = DAEMON_STOPPED;
				syslog(LOG_INFO, "sigchld: '%s' (pid: %d) killed by signal %d", daemon->name, info.si_pid, info.si_status);

				if (daemon->conf.start.killed == 1) {
					daemon_start(daemon);
				}
			} else {
				syslog(LOG_INFO, "sigchld: Orphan %d killed by signal %d", info.si_pid, info.si_status);
			}
			break;
		case CLD_DUMPED:
			if (daemon != NULL) {
				daemon->state = DAEMON_STOPPED;
				syslog(LOG_INFO, "sigchld: '%s' (pid: %d) dumped core", daemon->name, info.si_pid);

				if (daemon->conf.start.dumped == 1) {
					daemon_start(daemon);
				}
			} else {
				syslog(LOG_INFO, "sigchld: Orphan %d dumped core", info.si_pid);
			}
			break;
		default:
			abort();
		}
	}

	if (errno != 0) {
		syslog(LOG_ERR, "sigchld waitid: %m");
	}
}

/**
 * Setup all signal handlers, procmask, and returns @ref main loop's _pselect(2)_ mask.
 * @param[out] sigmaskp Signal mask used during @ref main loop's _pselect(2)_ call.
 */
void
signals_setup(sigset_t *sigmaskp) {
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
	sigemptyset(sigmaskp);
	sigaddset(sigmaskp, SIGTERM);
	sigaddset(sigmaskp, SIGHUP);
	sigaddset(sigmaskp, SIGCHLD);
	sigprocmask(SIG_SETMASK, sigmaskp, NULL);

	/* Init signal handlers */
	sigfillset(&action.sa_mask);

	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = sigterm_handler;
	sigaction(SIGTERM, &action, NULL);

	action.sa_flags = SA_RESTART;

	action.sa_handler = sighup_handler;
	sigaction(SIGHUP, &action, NULL);

	action.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &action, NULL);

	/* Define pselect's non blocked signals */
	sigfillset(sigmaskp);
	sigdelset(sigmaskp, SIGTERM);
	sigdelset(sigmaskp, SIGHUP);
	sigdelset(sigmaskp, SIGCHLD);
}

/** Modify signal process mask to avoid being interrupted by SIGCHLD. */
void
signals_stopping(void) {
	sigset_t unblock;

	sigemptyset(&unblock);
	sigaddset(&unblock, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &unblock, NULL);
}

/** Re-mask SIGCHLD to reap remaining processes. */
void
signals_ending(void) {
	sigset_t block;

	sigemptyset(&block);
	sigaddset(&block, SIGCHLD);
	sigprocmask(SIG_BLOCK, &block, NULL);
}
