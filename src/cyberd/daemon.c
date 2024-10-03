/* SPDX-License-Identifier: BSD-3-Clause */
#include "daemon.h"

#include "spawns.h"

#include <stdlib.h> /* abort, free, malloc */
#include <stdnoreturn.h> /* noreturn */
#include <sys/resource.h> /* setpriority */
#include <sys/stat.h> /* umask */
#include <syslog.h> /* syslog */
#include <unistd.h> /* close, chdir, setuid... */
#include <signal.h> /* sigemptyset, sigprocmask, kill */
#include <string.h> /* strdup */
#include <alloca.h> /* alloca */
#include <fcntl.h> /* open */
#include <err.h> /* err */

/**
 * Empty signal procmask of the current process.
 */
static int
daemon_child_sigprocmask(void) {
	sigset_t sigmask;

	if (sigemptyset(&sigmask) != 0) {
		return -1;
	}

	if (sigprocmask(SIG_SETMASK, &sigmask, NULL) != 0) {
		return -1;
	}

	return 0;
}

/**
 * Sets up the current process state according to @p conf.
 * @param name Name of the daemon.
 * @param conf Configuration of the daemon.
 * @returns Never, exits on failure to initialize process.
 */
static void noreturn
daemon_child_setup(const char *name, const struct daemon_conf *conf) {
	const char *in = conf->in,
	           *out = conf->out,
		   *workdir = conf->workdir;
	char **argv = conf->arguments, **envp = conf->environment;
	int infd, outfd, errfd;

	/********************
	 * Setting defaults *
	 ********************/

	if (conf->in == NULL) {
		in = CONFIG_DAEMON_DEV_NULL;
	}

	if (conf->out == NULL) {
		out = CONFIG_DAEMON_DEV_NULL;
	}

	if (conf->workdir == NULL) {
		workdir = CONFIG_DAEMON_DEFAULT_WORKDIR;
	}

	if (argv == NULL) {
		argv = alloca(sizeof (*argv) * 2);
		argv[0] = (char *)name;
		argv[1] = NULL;
	}

	if (envp == NULL) {
		envp = alloca(sizeof (*envp) * 1);
		envp[0] = NULL;
	}

	/*************************************
	 * Opening standard file descriptors *
	 *************************************/

	infd = open(in, O_RDONLY | O_CLOEXEC);
	if (infd < 0) {
		err(-1, "open '%s'", in);
	}

	outfd = open(out, O_WRONLY | O_CLOEXEC);
	if (outfd < 0) {
		err(-1, "open '%s'", out);
	}

	if (conf->err != NULL) {
		errfd = open(conf->err, O_WRONLY | O_CLOEXEC);
		if (errfd < 0) {
			err(-1, "open '%s'", conf->err);
		}
	} else {
		errfd = outfd;
	}

	/**************************************
	 * Replacing default file descriptors *
	 **************************************/

	if (dup2(infd, STDIN_FILENO) < 0) {
		err(-1, "dup2 STDIN_FILENO");
	}

	if (dup2(outfd, STDOUT_FILENO) < 0) {
		err(-1, "dup2 STDOUT_FILENO");
	}

	if (dup2(errfd, STDERR_FILENO) < 0) {
		err(-1, "dup2 STDERR_FILENO");
	}

	/***********************************************
	 * Parent process-inherited capabilities reset *
	 ***********************************************/

	umask(conf->umask);

	if (chdir(workdir) < 0) {
		err(-1, "chdir '%s'", workdir);
	}

	if (daemon_child_sigprocmask() < 0) {
		err(-1, "daemon_child_sigprocmask");
	}

	/**************************************
	 * Process credentials and scheduling *
	 **************************************/

	if (setuid(conf->uid) < 0) {
		err(-1, "setuid %d", conf->uid);
	}

	if (setgid(conf->gid) < 0) {
		err(-1, "setgid %d", conf->gid);
	}

	if (!conf->nosid && setsid() < 0) {
		err(-1, "setsid");
	}

	if (setpriority(PRIO_PROCESS, 0, conf->priority) != 0) {
		err(-1, "setpriority %d", conf->priority);
	}

	/********
	 * Exec *
	 ********/

	execve(conf->path, argv, envp);
	err(-1, "execve '%s'", conf->path);
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
		daemon->state = DAEMON_STARTED;
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

	if (daemon == NULL) {
		goto malloc_failure;
	}

	char * const copy = strdup(name);
	if (copy == NULL) {
		goto strdup_failure;
	}

	daemon->state = DAEMON_STOPPED;
	daemon->name = copy;
	daemon_conf_init(&daemon->conf);

	return daemon;
strdup_failure:
	free(daemon);
malloc_failure:
	return NULL;
}

/**
 * Frees every field of the struct, dereference
 * in spawns if not DAEMON_STOPPED, frees the structure
 * @param daemon A previously daemon_create()'d daemon
 */
void
daemon_destroy(struct daemon *daemon) {

	if (daemon->state != DAEMON_STOPPED) {
		/* Remove daemon index in spawns if previously spawned. */
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
	case DAEMON_STARTED:
		syslog(LOG_INFO, "daemon_start: '%s' already started", daemon->name);
		break;
	case DAEMON_STOPPED:
		if (daemon_spawn(daemon) == 0) {
			syslog(LOG_INFO, "daemon_start: '%s' started with pid: %d", daemon->name, daemon->pid);
		} else {
			syslog(LOG_INFO, "daemon_start: '%s' start failed", daemon->name);
		}
		break;
	case DAEMON_STOPPING:
		syslog(LOG_INFO, "daemon_start: '%s' is stopping", daemon->name);
		break;
	default:
		abort();
	}
}

/**
 * Send the finish signal if DAEMON_STARTED, and set it to DAEMON_STOPPING.
 * @param daemon Daemon to stop
 */
void
daemon_stop(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_STARTED:
		syslog(LOG_INFO, "daemon_stop: '%s' stopping with signal %d", daemon->name, daemon->conf.sigfinish);
		kill(daemon->pid, daemon->conf.sigfinish);
		daemon->state = DAEMON_STOPPING;
		break;
	case DAEMON_STOPPED:
		syslog(LOG_INFO, "daemon_stop: '%s' already stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		syslog(LOG_INFO, "daemon_stop: '%s' is stopping", daemon->name);
		break;
	default:
		abort();
	}
}

/**
 * Sends the configured signal for reconfiguration.
 * @param daemon Daemon to reload
 */
void
daemon_reload(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_STARTED:
		syslog(LOG_INFO, "daemon_reload: '%s' reloading with signal %d", daemon->name, daemon->conf.sigreload);
		kill(daemon->pid, daemon->conf.sigreload);
		break;
	case DAEMON_STOPPED:
		syslog(LOG_INFO, "daemon_reload: '%s' is stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		syslog(LOG_INFO, "daemon_reload: '%s' is stopping", daemon->name);
		break;
	default:
		abort();
	}
}

/**
 * Force kill of the process if not DAEMON_STOPPED
 * @param daemon Daemon to end
 */
void
daemon_end(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_STARTED:
		syslog(LOG_INFO, "daemon_end: '%s' was running, ending...", daemon->name);
		break;
	case DAEMON_STOPPED:
		syslog(LOG_INFO, "daemon_end: '%s' is stopped", daemon->name);
		return;
	case DAEMON_STOPPING:
		syslog(LOG_INFO, "daemon_end: '%s' ending...", daemon->name);
		break;
	default:
		abort();
	}

	kill(daemon->pid, SIGKILL);
}
