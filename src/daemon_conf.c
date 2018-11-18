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

	/* Default'ing daemon signal mask and handlers */
	sigset_t sigset;

	sigfillset(&sigset);
	posix_spawnattr_setflags(&conf->attr, POSIX_SPAWN_SETSIGDEF);
	posix_spawnattr_setsigdefault(&conf->attr, &sigset);

	sigemptyset(&sigset);
	posix_spawnattr_setflags(&conf->attr, POSIX_SPAWN_SETSIGMASK);
	posix_spawnattr_setsigmask(&conf->attr, &sigset);

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

