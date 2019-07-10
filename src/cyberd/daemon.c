#include "daemon.h"
#include "spawns.h"
#include "log.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

static int
daemon_child_close_all_proc(pid_t pid) {
	size_t buffersize = 16;

	while (buffersize < NAME_MAX) {
		char buffer[buffersize];

		if (snprintf(buffer, buffersize, "/proc/%d/fd", pid) < buffersize) {
			DIR *dirp = opendir(buffer);

			if (dirp != NULL) {
				struct dirent *entry;
		
				while ((errno = 0, entry = readdir(dirp)) != NULL) {
					char *end;
					unsigned long fd = strtoul(entry->d_name, &end, 10);
		
					if (*end == '\0') {
						close((int)fd);
					}
				}
		
				if (errno == 0) {
					closedir(dirp);
					return 0;
				}
			}

			break;
		} else {
			buffersize *= 2;
		}
	}

	return -1;
}

static int
daemon_child_close_all(void) {
	struct rlimit rlimit;

	if (getrlimit(RLIMIT_NOFILE, &rlimit) == 0) {
		for (int fd = 0; fd < rlimit.rlim_cur; fd++) {
			close(fd);
		}

		return 0;
	} else {
		warn("Unable to close all file descriptors");
		return -1;
	}
}

static int
daemon_child_setup_fds(struct daemon *daemon) {

	if (daemon_child_close_all_proc(daemon->pid) == -1) {
		return daemon_child_close_all();
	}

	return 0;
}

static int
daemon_child_setup_workdir(struct daemon *daemon) {

	if (daemon->conf.wd != NULL
		&& chdir(daemon->conf.wd) == -1) {
		warn("Unable to setup '%s' daemon working directory '%s', chdir", daemon->name, daemon->conf.wd);
		return -1;
	}

	return 0;
}

static int
daemon_child_setup_ids(struct daemon *daemon) {

	if (setuid(daemon->conf.uid) == -1) {
		warn("Unable to setup '%s' daemon user id at '%d'", daemon->name, daemon->conf.uid);
		return -1;
	}

	if (setgid(daemon->conf.gid) == -1) {
		warn("Unable to setup '%s' daemon group id at '%d'", daemon->name, daemon->conf.gid);
		return -1;
	}

	return 0;
}

static char **
daemon_child_argv(struct daemon *daemon) {

	if (daemon->conf.arguments != NULL) {
		return daemon->conf.arguments;
	} else {
		static char *emptyargv[] = { NULL, NULL };
		emptyargv[0] = daemon->name;

		return emptyargv;
	}
}

static char **
daemon_child_envp(struct daemon *daemon) {

	if (daemon->conf.environment != NULL) {
		return daemon->conf.environment;
	} else {
		static char *emptyenvp[] = { NULL };

		return emptyenvp;
	}
}

static void
daemon_child_setup(struct daemon *daemon) {

	umask(daemon->conf.umask);

	if (daemon_child_setup_fds(daemon) == 0
		&& daemon_child_setup_workdir(daemon) == 0
		&& daemon_child_setup_ids(daemon) == 0) {
		if (execve(daemon->conf.path,
			daemon_child_argv(daemon), daemon_child_envp(daemon)) == -1) {
			warn("Unable to execve '%s'", daemon->conf.path);
		}
	}
	/* Reached on error */
}

static int
daemon_spawn(struct daemon *daemon) {
	pid_t pid = fork();

	switch (pid) {
	case -1: /* failure */
		return -1;
	case 0: /* success-child */
		daemon->pid = getpid();
		daemon_child_setup(daemon);
		errx(EXIT_FAILURE, "Unable to setup daemon %s [%d]", daemon->name, daemon->pid);
	default: /* success-parent */
		daemon->pid = pid;
		daemon->state = DAEMON_RUNNING;
		spawns_record(daemon);
		return 0;
	}
}

struct daemon *
daemon_create(const char *name) {
	struct daemon *daemon = malloc(sizeof (*daemon));
	char *dupped = strdup(name);

	if (daemon != NULL && dupped != NULL) {
		daemon->name = dupped;
		daemon->state = DAEMON_STOPPED;

		daemon->namehash = hash_string(daemon->name);
		daemon->pid = 0;

		daemon_conf_init(&daemon->conf);

		return daemon;
	} else {
		free(daemon);
		free(dupped);

		return NULL;
	}
}

void
daemon_destroy(struct daemon *daemon) {

	if (daemon->state != DAEMON_STOPPED) {
		/* Remove daemon index in spawns if previously spawned */
		spawns_retrieve(daemon->pid);
	}

	daemon_conf_deinit(&daemon->conf);

	free(daemon->name);

	free(daemon);
}

void
daemon_start(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon start '%s': Already started", daemon->name);
		break;
	case DAEMON_STOPPED:
		if (daemon_spawn(daemon) == 0) {
			log_print("Daemon start '%s': Started with pid: %d",
				daemon->name, daemon->pid);
		} else {
			log_print("Daemon start '%s': Start failed", daemon->name);
		}
		break;
	case DAEMON_STOPPING:
		log_print("Daemon start '%s': Is stopping", daemon->name);
		break;
	default:
		log_error("Daemon start '%s': Inconsistent state", daemon->name);
		break;
	}
}

void
daemon_stop(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon stop '%s': Stopping...", daemon->name);
		kill(daemon->pid, daemon->conf.sigend);
		daemon->state = DAEMON_STOPPING;
		break;
	case DAEMON_STOPPED:
		log_print("Daemon stop '%s': Already stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("Daemon stop '%s': Is stopping", daemon->name);
		break;
	default:
		log_error("Daemon stop '%s': Inconsistent state", daemon->name);
		break;
	}
}

void
daemon_reload(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon reload '%s': Reloading...", daemon->name);
		kill(daemon->pid, daemon->conf.sigreload);
		break;
	case DAEMON_STOPPED:
		log_print("Daemon reload '%s': Is stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("Daemon reload '%s': Is stopping", daemon->name);
		break;
	default:
		log_error("Daemon reload '%s': Inconsistent state", daemon->name);
		break;
	}
}

void
daemon_end(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon end '%s': Was running, ending...", daemon->name);
		kill(daemon->pid, SIGKILL);
		break;
	case DAEMON_STOPPED:
		log_print("Daemon end '%s': Is stopped", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("Daemon end '%s': Ending...", daemon->name);
		kill(daemon->pid, SIGKILL);
		break;
	default:
		log_error("Daemon end '%s': Inconsistent state", daemon->name);
		break;
	}
}

