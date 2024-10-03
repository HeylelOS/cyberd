/* SPDX-License-Identifier: BSD-3-Clause */
#include "configuration.h"
#include "socket_switch.h"
#include "signals.h"
#include "spawns.h"
#include "daemon.h"

/**
 * @mainpage Cyberd init
 * Cyberd is a modern init system for unix-like operating systems.
 *
 * It aims at keeping things simple by just being a process leader for daemons.
 *
 * You can define daemons with configuration files, and use socket endpoints to
 * control aspects of their lifetime, such as starting, stopping, reloading or end them.
 *
 * Cyberd's init source code can be decomposed in three core components:
 * - The configuration: Where daemons are loaded/reloaded and configuration parsed.
 * - The socket switch: Where endpoints and connections are recorded and operated.
 * - The signal handlers: Where "process events" are received. Such as: system termination, configuration reloading and child processes reaping.
 *
 * The process runs in one of two-states: Signal-handling and non signal-handling.
 * Both modes have different signal masks to avoid spurious interruptions and potential collisions in data structures.
 * When non signal-handling (cf the @ref main loop in `src/cyberd/main.c`), all signals are blocked or ignored.
 * Only when the call to _pselect(2)_ is done, the signal masks changes and allows for handling of termination signals (_SIGTERM_),
 * configuration reload signal (_SIGHUP_) and child processes reaping (_SIGCHLD_).
 */

#include <stdlib.h> /* exit */
#include <stdnoreturn.h> /* noreturn */
#include <sys/reboot.h> /* reboot */
#include <sys/wait.h> /* waitpid */
#include <signal.h> /* kill */
#include <syslog.h> /* syslog, ... */
#include <libgen.h> /* basename */
#include <unistd.h> /* setsid, sync */
#include <errno.h> /* ECHILD, EINTR */

/**
 * Daemon reaped.
 * Logs informations about a reaped daemon,
 * and starts the daemon again if configured so.
 */
static void
reaped_daemon(const siginfo_t *info, struct daemon *daemon) {

	switch (info->si_code) {
	case CLD_EXITED:
		daemon->state = DAEMON_STOPPED;
		syslog(LOG_INFO, "'%s' (pid: %d) terminated with exit status %d", daemon->name, info->si_pid, info->si_status);
		if (info->si_status == 0) {
			if (daemon->conf.start.exitsuccess == 1) {
				daemon_start(daemon);
			}
		} else if (daemon->conf.start.exitfailure == 1) {
			daemon_start(daemon);
		}
		break;
	case CLD_KILLED:
		daemon->state = DAEMON_STOPPED;
		syslog(LOG_INFO, "'%s' (pid: %d) killed by signal %d", daemon->name, info->si_pid, info->si_status);
		if (daemon->conf.start.killed == 1) {
			daemon_start(daemon);
		}
		break;
	case CLD_DUMPED:
		daemon->state = DAEMON_STOPPED;
		syslog(LOG_INFO, "'%s' (pid: %d) dumped core", daemon->name, info->si_pid);
		if (daemon->conf.start.dumped == 1) {
			daemon_start(daemon);
		}
		break;
	default:
		abort();
	}
}

/**
 * Orphan reaped.
 * Logs informations about a reaped orphan process.
 */
static void
reaped_orphan(const siginfo_t *info) {

	switch (info->si_code) {
	case CLD_EXITED:
		syslog(LOG_INFO, "Orphan %d terminated with exit status %d", info->si_pid, info->si_status);
		break;
	case CLD_KILLED:
		syslog(LOG_INFO, "Orphan %d killed by signal %d", info->si_pid, info->si_status);
		break;
	case CLD_DUMPED:
		syslog(LOG_INFO, "Orphan %d dumped core", info->si_pid);
		break;
	default:
		abort();
	}
}

/**
 * Reap available children.
 * Perform a wait until all children or error.
 * @param options Forwarded to waitid(2). Usually 0 or WNOHANG, to avoid blocking if necessary.
 * @returns The last waitid(2) returned value, with errno in case of error.
 */
static int
reap_children(int options) {
	siginfo_t info;
	int status;

	options |= WEXITED;
	while ((status = waitid(P_ALL, 0, &info, options)) == 0 && info.si_pid != 0) {
		struct daemon * const daemon = spawns_retrieve(info.si_pid);

		if (daemon != NULL) {
			reaped_daemon(&info, daemon);
		} else {
			reaped_orphan(&info);
		}
	}

	return status;
}

#ifdef CONFIG_RC_PATH
/**
 * Run commands.
 * Run an early executable providing system initialization,
 * which may load some drivers, mount filesystems, etc...
 * @param path Path of the executable.
 */
static void
rc(const char *path) {
	static char * const argv [] = { "rc", NULL };
	/* It's not really important to call daemon_conf_init,
	 * what's important is that none of the start
	 * flags are set, so zero out the structure. */
	struct daemon daemon = {
		.state = DAEMON_STARTED,
		.name = *argv,
		.pid = fork(),
	};

	switch (daemon.pid) {
	case 0:
		execv(path, argv);
		syslog(LOG_ERR, "execv: %m");
		exit(-1);
	case -1:
		syslog(LOG_ERR, "fork: %m");
		return;
	default:
		break;
	}

	spawns_record(&daemon);

	/* Keep same signal mask during
	 * sigsuspend(2), except for SIGCHLD. */
	sigset_t sigmask;
	sigprocmask(0, NULL, &sigmask);
	sigdelset(&sigmask, SIGCHLD);
	do {
		sigsuspend(&sigmask);
		reap_children(WNOHANG);
	} while (!spawns_empty());
	/* Clean SIGCHLD marker. */
	sigchld = 0;
}
#endif

/**
 * Setup all subsystems.
 * Opens log subsystem. If configured, run commands.
 * Setup signal handlers. Create first endpoint.
 * And finally, load our configuration.
 * @param argc Arguments count.
 * @param argv Arguments values.
 * @param[out] sigmaskp Signals not blocked during _pselect(2)_.
 */
static void
setup(int argc, char **argv, sigset_t *sigmaskp) {

	setlogmask(
#ifndef NDEBUG
		LOG_UPTO(LOG_DEBUG)
#else
		LOG_UPTO(LOG_WARNING)
#endif
	);
	openlog(basename(*argv), LOG_PID | LOG_CONS | LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON);

	if (setsid() < 0) {
		syslog(LOG_ERR, "setsid: %m");
	}

	signals_setup(sigmaskp);
	socket_switch_setup(CONFIG_SOCKET_ENDPOINTS_PATH, CONFIG_SOCKET_ENDPOINTS_ROOT);

#ifdef CONFIG_RC_PATH
	rc(CONFIG_RC_PATH);
#endif

	configuration_load(CONFIG_CONFIGURATION_PATH);
}

/**
 * Teardown system, synchronizes filesystems to persistent storage.
 * Note some structures (configuration) may not be freed by default, because
 * they do not hold sensible system resources, such as locks, files...
 * @returns Never.
 */
static void noreturn
teardown(void) {
	sigset_t sigmask;

	/* Notify spawns they should stop. */
	syslog(LOG_NOTICE, "Stopping daemons...");
	spawns_stop();

	/* Destroying socket switch, to unlink endpoints. */
	socket_switch_teardown();
	/* Setup SIGALRM handler and planify timeout alarm. */
	signals_alarm(&sigmask, CONFIG_REBOOT_TIMEOUT);

	/* While there are children, the alarm didn't ring,
	 * and we still have daemons running, reap children. */
	bool children;
	do {
		sigsuspend(&sigmask);
		children = reap_children(WNOHANG) == 0 || errno != ECHILD;
	} while (children && !sigalrm && !spawns_empty());

	if (children) {
		/* Kill everyone left. This way, we are ready for @ref _sync(2)_. */
		syslog(LOG_NOTICE, "Ending remaining processes...");
		kill(-1, SIGKILL);
		/* Reap remaining children. */
		reap_children(0);
	}

#ifndef NDEBUG
	configuration_cleanup();
	spawns_cleanup();
#endif

	closelog();

	/* Synchronize all filesystems to disk(s),
	 * note: Standard specifies it may return before all syncs done.
	 * Let's hope the kernel's implementation will not perform
	 * reboot before the end of disk synchronization. */
	sync();

	reboot(sigreboot);
	/* We reached an error. */
	exit(EXIT_FAILURE);
}

/**
 * Main loop.
 * Initializes subsystems, operates socket switch while responding to signals.
 * @param argc Arguments count.
 * @param argv Arguments values.
 * @returns Never, as a returning init triggers a kernel panic.
 */
int
main(int argc, char *argv[]) {
	sigset_t sigmask;

	setup(argc, argv, &sigmask);

	do {
		fd_set *readfds;
		int fds = socket_switch_prepare(&readfds);

		errno = 0;
		fds = pselect(fds, readfds, NULL, NULL, NULL, &sigmask);

		if (fds >= 0) {
			socket_switch_operate(fds);
		} else if (errno == EINTR) {
			if (sighup) {
				configuration_reload();
				sighup = 0;
			}
			if (sigchld) {
				reap_children(WNOHANG);
				sigchld = 0;
			}
		} else {
			syslog(LOG_ERR, "pselect: %m");
		}
	} while (!sigreboot);

	teardown();
}
