#include "daemon_conf.h"

#include <stdlib.h>

void
daemon_conf_init(struct daemon_conf *conf) {

	conf->path = NULL;
	conf->arguments = NULL;

	posix_spawn_file_actions_init(&conf->file_actions);
	posix_spawnattr_init(&conf->attr);
}

void
daemon_conf_deinit(struct daemon_conf *conf) {

	free(conf->path);
	free(conf->arguments);

	posix_spawn_file_actions_destroy(&conf->file_actions);
	posix_spawnattr_destroy(&conf->attr);
}

