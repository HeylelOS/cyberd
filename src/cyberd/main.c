/* SPDX-License-Identifier: BSD-3-Clause */
#include "configuration.h"
#include "socket_switch.h"
#include "signals.h"
#include "spawns.h"

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
 * Only when the call to _pselect(2)_ is done, the signal masks changes and allows for handling of termination signals (_SIGTERM_, _SIGINT_ in debug),
 * configuration reload signal (_SIGHUP_) and child processes reaping (_SIGCHLD_).
 * Termination signals are not granted the _SA\_RESTART_ flag, so that the @ref main loop can detect a termination signal through an _EINTR_ _pselect(2)_ error.
 */

#include <stdlib.h> /* exit */
#include <stdnoreturn.h> /* noreturn */
#include <sys/reboot.h> /* reboot */
#include <sys/wait.h> /* waitpid */
#include <signal.h> /* kill */
#include <syslog.h> /* setlogmask, openlog, closelog, syslog */
#include <libgen.h> /* basename */
#include <unistd.h> /* setsid, sync */
#include <time.h> /* nanosleep */
#include <errno.h> /* EINTR */

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
	const pid_t pid = fork();
	int wstatus;

	switch (pid) {
	case 0:
		execv(path, argv);
		syslog(LOG_ERR, "execv: %m");
		exit(-1);
	case -1:
		syslog(LOG_ERR, "fork: %m");
		break;
	default:
		if (waitpid(pid, &wstatus, 0) < 0) {
			syslog(LOG_ERR, "waitpid %d: %m", pid);
		}
		if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) != 0) {
			syslog(LOG_ERR, "rc %s exited with status %d", path, WEXITSTATUS(wstatus));
		} else if (WIFSIGNALED(wstatus)) {
			syslog(LOG_ERR, "rc %s killed by signal %d", path, WTERMSIG(wstatus));
		} else if (WCOREDUMP(wstatus)) {
			syslog(LOG_ERR, "rc %s dumped core", path);
		}
		break;
	}
}
#endif

/**
 * Setup all core subsystems.
 * Opens log subsystem. If configured, run commands.
 * Setup signal handlers. Create first endpoint.
 * And finally, load our configuration.
 * @param argc Arguments count.
 * @param argv Arguments values.
 * @param[out] sigmaskp Signal mask not blocked during _pselect(2)_.
 */
static void
setup(int argc, char **argv, sigset_t *sigmaskp) {
	int mask = LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_ERR);

#ifndef NDEBUG
	mask |= LOG_MASK(LOG_DEBUG) | LOG_MASK(LOG_INFO);
#endif

	setlogmask(mask);
	openlog(basename(*argv), LOG_PID | LOG_CONS | LOG_NDELAY | LOG_NOWAIT, LOG_USER);

	if (setsid() < 0) {
		syslog(LOG_ERR, "setsid: %m");
	}

#ifdef CONFIG_RC_PATH
	rc(CONFIG_RC_PATH);
#endif

	signals_setup(sigmaskp);
	socket_switch_setup(CONFIG_SOCKET_ENDPOINTS_PATH, CONFIG_SOCKET_ENDPOINTS_ROOT);
	configuration_load(CONFIG_CONFIGURATION_PATH);
}

/**
 * Teardown system, synchronizes filesystems to persistent storage, daemons timeout is 5 seconds.
 * Note some structures (configuration) may not be freed by default, because
 * they do not hold sensible system resources, such as locks, files...
 * @returns Never.
 */
static void noreturn
teardown(void) {
	struct timespec req = {
		.tv_sec = 5,
		.tv_nsec = 0
	}, rem;

	syslog(LOG_INFO, "ending");

	/* Notify spawns they should stop */
	spawns_stop();
	/* Destroying socket switch, to unlink endpoints */
	socket_switch_teardown();
	/* Modifying the signal mask so we accept SIGCHLD now */
	signals_stopping();

	/* While there are spawns, and we didn't time out, we wait spawns for 5 seconds.
	 * In theory we sat SA_RESTART for SIGCHLD, but default signal behaviours might not be
	 * consistent upon implementations, so let's keep EINTR checked. */
	while (!spawns_empty() && nanosleep(&req, &rem) != 0 && errno == EINTR) {
		req = rem;
	}

	/* In case some processes didn't exit, we re-block SIGCHLD */
	signals_ending();
	/* And then SIGKILL and wait everyone left. */
	spawns_end();

	/* In release, really everyone left. This way, we are ready for @ref _sync(2)_. */
	kill(-1, SIGKILL);

	pid_t pid;
	while (pid = waitpid(-1, NULL, 0), pid > 0) {
		syslog(LOG_INFO, "Orphan process (pid: %d) force-ended", pid);
	}

#ifdef CONFIG_MEMORY_CLEANUP
	configuration_teardown();
#endif

	closelog();

	/* Synchronize all filesystems to disk(s),
	 * note: Standard specifies it may return before all syncs done */
	sync();

	reboot(rebootcmd);
	/* We reached an error */
	exit(EXIT_FAILURE);
}

/**
 * Main loop, initializes all subsystems, waits and operates socket switch while ensuring signal safety.
 * @param argc Arguments count.
 * @param argv Arguments values.
 * @returns Never, as a returning init triggers a kernel panic.
 */
int
main(int argc, char **argv) {
	sigset_t sigmask;

	setup(argc, argv, &sigmask);

	while (!rebootcmd) {
		fd_set *readfds, *writefds, *exceptfds;
		int fds = socket_switch_prepare(&readfds, &writefds, &exceptfds);

		errno = 0;
		fds = pselect(fds, readfds, writefds, exceptfds, NULL, &sigmask);

		if (fds >= 0) {
			socket_switch_operate(fds);
		} else if (errno != EINTR) {
			syslog(LOG_ERR, "pselect: %m");
		}

		if (rebootcmd == RB_SW_SUSPEND) {
			if (reboot(RB_SW_SUSPEND) != 0) {
				syslog(LOG_ERR, "reboot(RB_SW_SUSPEND): %m");
			}
			rebootcmd = 0;
		}
	}

	teardown();
}
