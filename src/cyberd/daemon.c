/* SPDX-License-Identifier: BSD-3-Clause */
#include "daemon.h"

#include "spawns.h"
#include "log.h"

#include "config.h"

#include <stdlib.h> /* free, exit, ... */
#include <stdint.h> /* INT32_MAX */
#include <stdnoreturn.h> /* noreturn */
#include <sys/resource.h> /* setpriority */
#include <sys/stat.h> /* umask */
#include <unistd.h> /* close, chdir, setuid... */
#include <signal.h> /* kill */
#include <string.h> /* strdup */
#include <errno.h> /* errno */
#include <err.h> /* err, warn, ... */

#ifdef CONFIG_HAS_PROCFS
#include <dirent.h> /* opendir, ... */
#endif

/**
 * Closes all opened file descriptors of the current process.
 */
static int
daemon_child_close_all(void) {
#ifdef CONFIG_HAS_PROCFS
	DIR * const dirp = opendir("/proc/self/fd");
	int retval = 0;

	if (dirp != NULL) {
		struct dirent *entry;
		char *end;

		while (errno = 0, entry = readdir(dirp), entry != NULL) {
			unsigned long fd = strtoul(entry->d_name, &end, 10);

			if (*end == '\0' && fd < INT32_MAX && fd != dirfd(dirp)) {
				close((int)fd);
			}
		}

		if (errno != 0) {
			warn("readdir");
			retval = -1;
		}

		closedir(dirp);
	} else {
		warn("opendir('/proc/self/fd')");
		retval = -1;
	}

	return retval;
#else
	const int fdmax = (int)sysconf(_SC_OPEN_MAX); /* Truncation should not be a problem here */

	if (fdmax < 0) {
		warn("sysconf(_SC_OPEN_MAX)");
		return -1;
	}

	for (int fd = 0; fd <= fdmax; fd++) {
		close(fd);
	}

	return 0;
#endif
}

/**
 * Returns an argument array suitable for _execve(2)_.
 * @param name Name of the daemon used as a fallback if @p conf doesn't have a configured argument array.
 * @param conf Configuration from which we retrieve the argument array if available.
 * @returns Argument array.
 */
static char **
daemon_child_argv(const char *name, const struct daemon_conf *conf) {

	if (conf->arguments != NULL) {
		return conf->arguments;
	} else {
		static char *emptyargv[2];

		emptyargv[0] = (char *)name;
		emptyargv[1] = NULL;

		return emptyargv;
	}
}

/**
 * Returns an environment array suitable for _execve(2)_.
 * @param conf Configuration from which we retrieve the environment array if available.
 * @returns Environment array.
 */
static char **
daemon_child_envp(const struct daemon_conf *conf) {

	if (conf->environment != NULL) {
		return conf->environment;
	} else {
		static char *emptyenvp[] = { NULL };

		return emptyenvp;
	}
}

/**
 * Sets up the current process state according to @p conf.
 * @param name Name of the daemon.
 * @param conf Configuration of the daemon.
 * @returns Never, exits on failure to initialize process.
 */
static void noreturn
daemon_child_setup(const char *name, const struct daemon_conf *conf) {

	umask(conf->umask);

	if (daemon_child_close_all() != 0) {
		exit(EXIT_FAILURE);
	}

	if (conf->workdir != NULL && chdir(conf->workdir) != 0) {
		err(EXIT_FAILURE, "chdir '%s'", conf->workdir);
	}

	if (setuid(conf->uid) != 0) {
		err(EXIT_FAILURE, "setuid %d", conf->uid);
	}

	if (setgid(conf->gid) != 0) {
		err(EXIT_FAILURE, "setgid %d", conf->gid);
	}

	if (setsid() < 0) {
		err(EXIT_FAILURE, "setsid");
	}

	if (setpriority(PRIO_PROCESS, 0, conf->priority) != 0) {
		err(EXIT_FAILURE, "setpriority %d", conf->priority);
	}

	execve(conf->path, daemon_child_argv(name, conf), daemon_child_envp(conf));
	err(EXIT_FAILURE, "execve '%s'", conf->path);
}

/**
 * Spawns the process, doesn't return from the child.
 * @param daemon Daemon to spawn.
 * @returns Doesn't return from child. In the parent: zero if successful, non-zero on error to _fork(2)_.
 */
static int
daemon_spawn(struct daemon *daemon) {
	const pid_t pid = fork();

	switch (pid) {
	case -1: /* failure */
		return -1;
	case 0: /* success-child */
		daemon_child_setup(daemon->name, &daemon->conf);
	default: /* success-parent */
		daemon->pid = pid;
		daemon->state = DAEMON_RUNNING;
		spawns_record(daemon);
		return 0;
	}
}

/**
 * Allocates a new daemon
 * @param name Daemon's identifier, string internally copied
 * @return Newly allocated daemon
 */
struct daemon *
daemon_create(const char *name) {
	struct daemon * const daemon = malloc(sizeof (*daemon));
	char * const dupped = strdup(name);

	if (daemon != NULL && dupped != NULL) {

		daemon->state = DAEMON_STOPPED;

		daemon->name = dupped;
		daemon->pid = 0;

		daemon_conf_init(&daemon->conf);

		return daemon;
	} else {
		free(daemon);
		free(dupped);
		return NULL;
	}
}

/**
 * Frees every field of the struct, dereference
 * in spawns if not DAEMON_STOPPED, frees the structure
 * @param daemon A previously daemon_create()'d daemon
 */
void
daemon_destroy(struct daemon *daemon) {

	if (daemon->state != DAEMON_STOPPED) {
		/* Remove daemon index in spawns if previously spawned */
		spawns_retrieve(daemon->pid);
	}
	free(daemon->name);
	daemon_conf_deinit(&daemon->conf);

	free(daemon);
}

/**
 * Spawns a daemon if it was DAEMON_STOPPED.
 * If successful, records it into spawns, else does nothing.
 * @param daemon Daemon to start
 */
void
daemon_start(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_info("daemon_start: '%s' already started", daemon->name);
		break;
	case DAEMON_STOPPED:
		if (daemon_spawn(daemon) == 0) {
			log_info("daemon_start: '%s' started with pid: %d", daemon->name, daemon->pid);
		} else {
			log_info("daemon_start: '%s' start failed", daemon->name);
		}
		break;
	case DAEMON_STOPPING:
		log_info("daemon_start: '%s' is stopping", daemon->name);
		break;
	default:
		log_error("daemon_start: '%s' is in an inconsistent state", daemon->name);
		break;
	}
}

/**
 * Send the finish signal if DAEMON_RUNNING, and set it to DAEMON_STOPPING.
 * @param daemon Daemon to stop
 */
void
daemon_stop(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_info("daemon_stop: '%s' stopping with signal %d", daemon->name, daemon->conf.sigfinish);
		kill(daemon->pid, daemon->conf.sigfinish);
		daemon->state = DAEMON_STOPPING;
		break;
	case DAEMON_STOPPED:
		log_info("daemon_stop: '%s' already stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_info("daemon_stop: '%s' is stopping", daemon->name);
		break;
	default:
		log_error("daemon_stop: '%s' is in an inconsistent state", daemon->name);
		break;
	}
}

/**
 * Sends the configured signal for reconfiguration.
 * @param daemon Daemon to reload
 */
void
daemon_reload(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_info("daemon_reload: '%s' reloading with signal %d", daemon->name, daemon->conf.sigreload);
		kill(daemon->pid, daemon->conf.sigreload);
		break;
	case DAEMON_STOPPED:
		log_info("daemon_reload: '%s' is stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_info("daemon_reload: '%s' is stopping", daemon->name);
		break;
	default:
		log_error("daemon_reload: '%s' is in and inconsistent state", daemon->name);
		break;
	}
}

/**
 * Force kill of the process if not DAEMON_STOPPED
 * @param daemon Daemon to end
 */
void
daemon_end(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_info("daemon_end: '%s' was running, ending...", daemon->name);
		kill(daemon->pid, SIGKILL);
		break;
	case DAEMON_STOPPED:
		log_info("daemon_end: '%s' is stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_info("daemon_end: '%s' ending...", daemon->name);
		kill(daemon->pid, SIGKILL);
		break;
	default:
		log_error("daemon_end: '%s' is in an inconsistent state", daemon->name);
		break;
	}
}
