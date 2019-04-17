#include "daemon_conf.h"

#include <stdlib.h>
#include <signal.h>

void
daemon_conf_init(struct daemon_conf *conf) {

	conf->path = NULL;
	conf->arguments = NULL;
	conf->sigend = SIGTERM;
	conf->sigreload = SIGHUP;

	posix_spawn_file_actions_init(&conf->file_actions);
	posix_spawnattr_init(&conf->attr);

	/*
	No need to redefine signals defaults because we unmask everyone,
	and the posix_spawn standard specifies that:
	"Signals set to be caught by the calling process shall be set
	to the default action in the child process."
	*/
	sigset_t sigmask;
	sigemptyset(&sigmask);
	posix_spawnattr_setflags(&conf->attr, POSIX_SPAWN_SETSIGMASK);
	posix_spawnattr_setsigmask(&conf->attr, &sigmask);

	/* Zero'ing startmask */
	conf->startmask = 0;
}

void
daemon_conf_deinit(struct daemon_conf *conf) {

	free(conf->path);
	free(conf->arguments);

	posix_spawn_file_actions_destroy(&conf->file_actions);
	posix_spawnattr_destroy(&conf->attr);
}

