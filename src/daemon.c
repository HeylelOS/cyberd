#include "daemon.h"
#include "spawns.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct daemon *
daemon_create(const char *name) {
	struct daemon *daemon = malloc(sizeof(*daemon));

	daemon->name = strdup(name);
	daemon->state = DAEMON_STOPPED;

	daemon->namehash = hash_string(name);
	daemon->pid = 0;

	daemon_conf_init(&daemon->conf);

	return daemon;
}

void
daemon_destroy(struct daemon *daemon) {

	free(daemon->name);

	daemon_conf_deinit(&daemon->conf);
}

void
daemon_start(struct daemon *daemon) {

	switch(daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s start]: Already started\n", daemon->name);
		break;
	case DAEMON_STOPPED: {
		pid_t pid;

		log_print("[daemon %s start]: Starting...\n", daemon->name);

		pid = fork();

		if(pid == 0) {
			extern char **environ;

			if(daemon->conf.arguments == NULL) {
				daemon->conf.arguments = alloca(2 * sizeof(char *));
				daemon->conf.arguments[0] = daemon->name;
				daemon->conf.arguments[1] = NULL;
			}

			execve(daemon->conf.file,
				daemon->conf.arguments,
				environ);

			exit(EXIT_FAILURE);
		} else {
			if(pid > 0) {

				daemon->pid = pid;
				spawns_record(daemon);
				log_print("    [daemon forked] with pid: %d\n", pid);
			} else {
				log_error("    [daemon fork]");
			}
		}
	} break;
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

	switch(daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s stop]: Stopping...\n", daemon->name);
		/*
			kill(daemon->pid, daemon->sigend);
			daemon->state = DAEMON_STOPPING;
		*/
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

	switch(daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s reload]: Reloading...\n", daemon->name);
		/*
			kill(daemon->pid, daemon->sigreload);
		*/
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

	switch(daemon->state) {
	case DAEMON_RUNNING:
		log_print("[daemon %s end]: Was running, ending...\n", daemon->name);
		/*
			kill(daemon->pid, 9);
		*/
		break;
	case DAEMON_STOPPED:
		log_print("[daemon %s end]: Is stopped\n", daemon->name);
		break;
	case DAEMON_STOPPING:
		log_print("[daemon %s end]: Ending...\n", daemon->name);
		/*
			kill(daemon->pid, 9);
		*/
		break;
	default:
		log_print("[daemon %s end]: Inconsistent state\n", daemon->name);
		break;
	}
}

