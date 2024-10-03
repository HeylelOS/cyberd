/* SPDX-License-Identifier: BSD-3-Clause */
#include "signals.h"

#include <sys/reboot.h> /* reboot */
#include <assert.h> /* static_assert */
#include <unistd.h> /* alarm */
#include <stddef.h> /* NULL */

volatile sig_atomic_t sigreboot, sigchld, sighup, sigalrm;

/**
 * SIGTERM. Checks for additional informations from a potential _sigqueue(2)_
 * to determine which _reboot(2)_ action to engage.
 * @param siginfo Signal informations.
 */
static void
sigterm_handler(int, siginfo_t *siginfo, void *) {

	/* Obviously, the @ref main loop is broken
	 * if any one of these supported actions equals zero. */
	static_assert (RB_AUTOBOOT != 0);
	static_assert (RB_HALT_SYSTEM != 0);
	static_assert (RB_POWER_OFF != 0);
	static_assert (RB_SW_SUSPEND != 0);

	if (siginfo->si_code == SI_QUEUE) {
		const int value = siginfo->si_value.sival_int;

		switch (value) {
		case RB_AUTOBOOT:    [[fallthrough]];
		case RB_HALT_SYSTEM: [[fallthrough]];
		case RB_POWER_OFF:
			sigreboot = value;
			break;
		case RB_SW_SUSPEND:
			/* Unsupported suspend. */
			break;
		default:
			/* Invalid reboot magic. */
			break;
		}
	} else {
		sigreboot = RB_POWER_OFF;
	}
}

/** SIGCHLD handler, child processes reaping. */
static void
sigchld_handler(int) {
	sigchld = 1;
}

/** SIGHUP handler, daemons configurations. */
static void
sighup_handler(int) {
	sighup = 1;
}

/** SIGALRM handler, teardown timeout alarm. */
static void
sigalrm_handler(int) {
	sigalrm = 1;
}

/**
 * Setup signal handlers, procmask, and returns @ref main loop's _pselect(2)_.
 * @param[out] sigmaskp Signal mask used during @ref main loop's _pselect(2)_.
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

	/* Process signal mask. */
	sigemptyset(sigmaskp);
	sigaddset(sigmaskp, SIGTERM);
	sigaddset(sigmaskp, SIGCHLD);
	sigaddset(sigmaskp, SIGHUP);
	sigprocmask(SIG_SETMASK, sigmaskp, NULL);

	/* All signals blocked during signal handlers. */
	sigfillset(&action.sa_mask);

	/* SIGTERM needs SA_SIGINFO for sigqueue(3) value. */
	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = sigterm_handler;
	sigaction(SIGTERM, &action, NULL);

	/* Neither SIGCHLD nor SIGHUP require SA_SIGINFO. */
	action.sa_flags = 0;

	action.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &action, NULL);

	action.sa_handler = sighup_handler;
	sigaction(SIGHUP, &action, NULL);

	/* Delivery signal mask. */
	sigemptyset(sigmaskp);
}

/**
 * Timeout alarm.
 * Sets the process mask to block SIGALRM,
 * setup its signal handler and planify an alarm.
 * @param[out] sigmaskp Signal mask used during @ref teardown loop's _sigsuspend(2)_.
 */
void
signals_alarm(sigset_t *sigmaskp, unsigned int seconds) {
	struct sigaction action;
	sigset_t oldmask;

	/* Add SIGALRM to process signal mask. */
	sigemptyset(sigmaskp);
	sigaddset(sigmaskp, SIGALRM);
	sigprocmask(SIG_BLOCK, sigmaskp, &oldmask);

	/* Set SIGALRM signal handler. */
	sigfillset(&action.sa_mask);
	action.sa_handler = sigalrm_handler;
	sigaction(SIGALRM, &action, NULL);

	/* Set timeout alarm. */
	alarm(seconds);

	/* Delivery signal mask. */
	sigdelset(&oldmask, SIGCHLD);
	*sigmaskp = oldmask;
}
