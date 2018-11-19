#include "daemon.h"
#include "spawns.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

struct daemon *
daemon_create(const char *name) {
	struct daemon *daemon = malloc(sizeof (*daemon));

	daemon->name = strdup(name);
	daemon->state = DAEMON_STOPPED;

	daemon->namehash = hash_string(name);
	daemon->pid = 0;

	daemon_conf_init(&daemon->conf);

	return daemon;
}

void
daemon_destroy(struct daemon *daemon) {

	if (daemon->state != DAEMON_STOPPED) {
		/* Remove daemon index in spawns if
		previously spawned */
		spawns_retrieve(daemon->pid);
	}

	free(daemon->name);

	daemon_conf_deinit(&daemon->conf);

	free(daemon);
}

void
daemon_start(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon start '%s': Already started\n", daemon->name);
		break;
	case DAEMON_STOPPED: {
		extern char **environ;
		char **arguments = daemon->conf.arguments;
		int errcode;

		if (arguments == NULL) {
			arguments = alloca(2 * sizeof (char *));
			arguments[0] = daemon->name;
			arguments[1] = NULL;
		}

		if ((errcode = posix_spawn(&daemon->pid, daemon->conf.path,
			&daemon->conf.file_actions, &daemon->conf.attr,
			arguments, environ)) == 0) {

			daemon->state = DAEMON_RUNNING;
			spawns_record(daemon);

			log_print("Daemon start '%s': Started with pid: %d\n",
				daemon->name, daemon->pid);
		} else {
			log_print("Daemon start '%s': Start failed: %s\n",
				daemon->name, strerror(errcode));
		}
	} break;
	case DAEMON_STOPPING:
		log_print("Daemon start '%s': Is stopping\n", daemon->name);
		break;
	default:
		log_print("Daemon start '%s': Inconsistent state\n", daemon->name);
		break;
	}
}

void
daemon_stop(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon stop '%s': Stopping...\n", daemon->name);
		kill(daemon->pid, daemon->conf.sigend);
		daemon->state = DAEMON_STOPPING;
		break;
	case DAEMON_STOPPED:
		log_print("Daemon stop '%s': Already stopped\n", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("Daemon stop '%s': Is stopping\n", daemon->name);
		break;
	default:
		log_print("Daemon stop '%s': Inconsistent state\n", daemon->name);
		break;
	}
}

void
daemon_reload(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon reload '%s': Reloading...\n", daemon->name);
		kill(daemon->pid, daemon->conf.sigreload);
		break;
	case DAEMON_STOPPED:
		log_print("Daemon reload '%s': Is stopped\n", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("Daemon reload '%s': Is stopping\n", daemon->name);
		break;
	default:
		log_print("Daemon reload '%s': Inconsistent state\n", daemon->name);
		break;
	}
}

void
daemon_end(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("Daemon end '%s': Was running, ending...\n", daemon->name);
		kill(daemon->pid, SIGKILL);
		break;
	case DAEMON_STOPPED:
		log_print("Daemon end '%s': Is stopped\n", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("Daemon end '%s': Ending...\n", daemon->name);
		kill(daemon->pid, SIGKILL);
		break;
	default:
		log_print("Daemon end '%s': Inconsistent state\n", daemon->name);
		break;
	}
}

