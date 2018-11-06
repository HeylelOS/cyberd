#include "daemon_conf.h"

#include <stdlib.h>

void
daemon_conf_init(struct daemon_conf *conf) {

	conf->file = NULL;
	conf->arguments = NULL;
}

void
daemon_conf_deinit(struct daemon_conf *conf) {

	free(conf->file);
	free(conf->arguments);
}

