#include "log.h"
#include "signals.h"
#include "configuration.h"
#include "spawns/spawns.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>

static void
sigterm_handler(int sig) {
	extern bool running;

	running = false;
}

static void
sighup_handler(int sig) {

	configuration(CONFIG_RELOAD);
}

static void
sigchld_handler(int sig) {
	extern struct spawns spawns;
	int status;
	pid_t child = waitpid(-1, &status, WNOHANG);
	struct daemon *daemon = spawns_retrieve(&spawns, child);

	if (daemon != NULL) {
		log_print("Daemon %s (pid: %d) terminated with status: %d\n",
			daemon->name, child, status);
	} else {
		log_print("Orphan process %d terminated\n", child);
	}
}

void
init_signals(sigset_t *set) {
	struct sigaction action;

	/* Define process' blocked signals */
	sigfillset(set);
	sigprocmask(SIG_SETMASK, set, NULL);

	/* Init signal handlers */
	sigfillset(&action.sa_mask);
	action.sa_flags = 0;

	action.sa_handler = sigterm_handler;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	action.sa_handler = sighup_handler;
	sigaction(SIGHUP, &action, NULL);

	action.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &action, NULL);

	/* Define pselect's non blocked signals */
	sigdelset(set, SIGTERM);
	sigdelset(set, SIGINT);
	sigdelset(set, SIGHUP);
	sigdelset(set, SIGCHLD);
}

