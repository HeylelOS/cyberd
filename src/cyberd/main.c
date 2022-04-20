/* SPDX-License-Identifier: BSD-3-Clause */
#include "configuration.h"
#include "socket_switch.h"
#include "signals.h"
#include "spawns.h"
#include "log.h"

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
#include <sys/stat.h> /* umask */
#include <unistd.h> /* sync */
#include <time.h> /* nanosleep */
#include <errno.h> /* EINTR */

#ifdef NDEBUG
#include <sys/wait.h> /* waitpid */
#include <signal.h> /* kill */
#endif

/**
 * Setup all core subsystems.
 * Initialize umask to something common. Sanity check our pid and user id.
 * Opens log subsystem with process name. Setup signal handlers. Create first endpoint.
 * And finally, load our configuration.
 * @param argc Arguments count.
 * @param argv Arguments values.
 * @param[out] sigmaskp Signal mask not blocked during _pselect(2)_.
 */
static void
setup(int argc, char **argv, sigset_t *sigmaskp) {

	umask(022);

#ifdef NDEBUG
	if (getpid() != 1) {
		errx(EXIT_FAILURE, "Must be run as pid 1");
	}

	if (setuid(0) != 0) {
		errx(EXIT_FAILURE, "Must be run as root");
	}
#endif

	log_setup(*argv);
	signals_setup(sigmaskp);
	socket_switch_setup();
	configuration_load();
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

	log_info("ending");

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

#ifdef NDEBUG
	/* In release, really everyone left. This way, we are ready for @ref _sync(2)_. */
	kill(-1, SIGKILL);

	pid_t pid;
	while (pid = waitpid(-1, NULL, 0), pid > 0) {
		log_info("Orphan process (pid: %d) force-ended", pid);
	}
#endif

#ifdef CONFIG_MEMORY_CLEANUP
	configuration_teardown();
#endif

	log_teardown();

	/* Synchronize all filesystems to disk(s),
	 * note: Standard specifies it may return before all syncs done */
	sync();

	reboot(signals_requested_reboot);
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

	log_info("running");

	while (!signals_requested_reboot) {
		fd_set *readfds, *writefds, *exceptfds;
		int fds = socket_switch_prepare(&readfds, &writefds, &exceptfds);

		errno = 0;
		fds = pselect(fds, readfds, writefds, exceptfds, NULL, &sigmask);

		if (fds >= 0) {
			socket_switch_operate(fds);
		} else if (errno != EINTR) {
			log_error("pselect: %m");
		}
	}

	teardown();
}
