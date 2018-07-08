#include "log.h"
#include "daemon.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void
daemon_init(struct daemon *daemon,
	const char *name) {

	daemon->name = strdup(name);
	daemon->hash = daemon_hash(name);
	daemon->state = DAEMON_STOPPED;

	daemon->file = NULL;
	daemon->arguments = NULL;
}

void
daemon_destroy(struct daemon *daemon) {

	free(daemon->name);

	free(daemon->file);
	free(daemon->arguments);
}

/* Should implement a SipHash, FNV hash will do temporarily */

#define FNV_OFFSET_BASIS	0xcbf29ce484222325
#define FNV_PRIME		0x100000001b3

hash_t
daemon_hash(const char *name) {
	const uint8_t *ptr = (const uint8_t *)name;
	const uint8_t *end = (const uint8_t *)name + strlen(name);
	hash_t hash = FNV_OFFSET_BASIS;

	while (ptr != end) {
		hash *= FNV_PRIME;
		hash ^= *ptr;

		++ptr;
	}

	return hash;
}

void
daemon_start(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s start]: Already started\n", daemon->name);
		break;
	case DAEMON_STOPPED: {
			pid_t pid;

			log_print("[daemon %s start]: Starting...\n", daemon->name);

			pid = fork();

			if (pid == 0) {
				extern char **environ;

				if (daemon->arguments == NULL) {
					daemon->arguments = alloca(2 * sizeof (char *));
					daemon->arguments[0] = daemon->name;
					daemon->arguments[1] = NULL;
				}

				execve(daemon->file, daemon->arguments, environ);

				exit(EXIT_FAILURE);
			} else {
				if (pid > 0) {
					log_print("    [daemon forked] with pid: %d\n", pid);
				} else {
					log_error("    [daemon fork]");
				}
			}
		}
		break;
	case DAEMON_STOPPING:
		log_print("[daemon %s start]: Is stopping\n", daemon->name);
		break;
	default:
		log_print("[daemon %s start]: Inconsistent state\n", daemon->name);
		break;
	}
}

void
daemon_stop(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s stop]: Stopping...\n", daemon->name);
		break;
	case DAEMON_STOPPED:
		log_print("[daemon %s stop]: Already stopped\n", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("[daemon %s stop]: Is stopping\n", daemon->name);
		break;
	default:
		log_print("[daemon %s stop]: Inconsistent state\n", daemon->name);
		break;
	}
}

void
daemon_reload(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s reload]: Reloading...\n", daemon->name);
		break;
	case DAEMON_STOPPED:
		log_print("[daemon %s reload]: Is stopped\n", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("[daemon %s reload]: Is stopping\n", daemon->name);
		break;
	default:
		log_print("[daemon %s reload]: Inconsistent state\n", daemon->name);
		break;
	}
}

void
daemon_end(struct daemon *daemon) {

	switch (daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s end]: Was running, ending...\n", daemon->name);
		break;
	case DAEMON_STOPPED:
		log_print("[daemon %s end]: Is stopped\n", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("[daemon %s end]: Ending...\n", daemon->name);
		break;
	default:
		log_print("[daemon %s end]: Inconsistent state\n", daemon->name);
		break;
	}
}

