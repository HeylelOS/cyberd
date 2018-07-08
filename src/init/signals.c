#include "log.h"
#include "init.h"
#include "signals.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

static void
sigterm_handler(int sig) {
	init.running = false;
}

static void
sigchld_handler(int sig) {
	int status;
	pid_t child = waitpid(-1, &status, WNOHANG);

	log_print("Child %d terminated\n", child);
}

void
init_signals(void) {
	struct sigaction action;

	sigfillset(&action.sa_mask);
	action.sa_flags = 0;

	action.sa_handler = sigterm_handler;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	action.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &action, NULL);
}

