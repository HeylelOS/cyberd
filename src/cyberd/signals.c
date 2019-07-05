#include "signals.h"
#include "configuration.h"
#include "spawns.h"
#include "log.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>

static sigset_t initsigset;

static void
sigterm_handler(int sig) {
	extern bool running;

	running = false;
}

static void
sighup_handler(int sig) {

	configuration_reload();
}

static void
sigchld_handler(int sig) {
	int status;
	pid_t child;

	while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
		struct daemon *daemon = spawns_retrieve(child);

		if (daemon != NULL) {
			daemon->state = DAEMON_STOPPED;

			log_print("Daemon %s (pid: %d) terminated with status: %d\n",
				daemon->name, child, status);
		} else {
			log_print("Orphan process %d terminated\n", child);
		}
	}
}

void
signals_init(void) {
	struct sigaction action;

	/* Define process' blocked signals */
	sigfillset(&initsigset);
	sigprocmask(SIG_SETMASK, &initsigset, NULL);

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
	sigdelset(&initsigset, SIGTERM);
	sigdelset(&initsigset, SIGINT);
	sigdelset(&initsigset, SIGHUP);
	sigdelset(&initsigset, SIGCHLD);
}

const sigset_t *
signals_sigset(void) {

	return &initsigset;
}

void
signals_stopping(void) {
	sigset_t unblock;

	sigemptyset(&unblock);
	sigaddset(&unblock, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &unblock, NULL);
}

void
signals_ending(void) {
	sigset_t block;

	sigemptyset(&block);
	sigaddset(&block, SIGCHLD);
	sigprocmask(SIG_BLOCK, &block, NULL);
}

