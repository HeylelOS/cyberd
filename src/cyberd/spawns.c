/* SPDX-License-Identifier: BSD-3-Clause */
#include "spawns.h"

#include "daemon.h"
#include "tree.h"

#include <syslog.h> /* syslog */

/**
 * Spawns tree comparison function. Identifies by the daemon's pid.
 * @param lhs Left hand side operand.
 * @param rhs Right hand side operand.
 * @returns The comparison between @p lhs and @p rhs pids.
 */
static int
spawns_compare_function(const tree_element_t *lhs, const tree_element_t *rhs) {
	const struct daemon * const ldaemon = lhs, * const rdaemon = rhs;
	return ldaemon->pid - rdaemon->pid;
}

/**
 * Spawns storage. All nodes' element belong to `src/cyberd/configuration.c`.
 * To avoid memory usage mishaps, spawns manipulation is basically restricted to three components:
 * - `src/cyberd/main.c`: During @ref teardown, to ensure the respect of the timeout from child processes.
 * - `src/cyberd/signals.c`: During @ref sigchld_handler, __ONLY__ when reaping a process, if it indeed is a daemon.
 * - `src/cyberd/daemon.c`: All other states, ensuring a @ref daemon_start spawns something, and @ref daemon_destroy doesn't left invalid nodes.
 * This storage is indexed using the daemon's pid as identifiers. All daemon should be in a non-@ref DAEMON_STOPPED state.
 */
static struct tree spawns = { .compare = spawns_compare_function };

/**
 * Register a running daemon.
 * @param daemon A daemon which must not be @ref DAEMON_STOPPED.
 */
void
spawns_record(struct daemon *daemon) {
	tree_insert(&spawns, daemon);
}

/**
 * Retrieve a spawned daemon.
 * @param pid Pid of the running daemon, if any is associated.
 * @returns The daemon if spawned, _NULL_ if none found.
 */
struct daemon *
spawns_retrieve(pid_t pid) {
	const struct daemon element = { .pid = pid };
	return tree_remove(&spawns, &element);
}

/**
 * Check if there are no more daemons left.
 * @returns true if empty, false else.
 */
bool
spawns_empty(void) {
	return spawns.root == NULL;
}

/**
 * Deactivate possible respawns, and politely ask for termination.
 * @param element Daemon.
 */
static void
spawns_stop_element(tree_element_t *element) {
	struct daemon * const daemon = element;

	/* It's important to disallow daemons' restart when stopping */
	daemon->conf.start.load = 0;
	daemon->conf.start.reload = 0;
	daemon->conf.start.exitsuccess = 0;
	daemon->conf.start.exitfailure = 0;
	daemon->conf.start.killed = 0;
	daemon->conf.start.dumped = 0;

	daemon_stop(daemon);
}

/** Deactivate daemon's spawning and stop all of them */
void
spawns_stop(void) {
	tree_mutate(&spawns, spawns_stop_element);
}

#ifndef NDEBUG
void
spawns_cleanup(void) {
	tree_deinit(&spawns);
}
#endif
