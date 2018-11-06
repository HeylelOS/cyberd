#ifndef DAEMON_CONF_H
#define DAEMON_CONF_H

#include <stdio.h>
#include <stdbool.h>

/* struct to contain configurations */
struct daemon_conf {
	char *file;
	char **arguments;
};

void
daemon_conf_init(struct daemon_conf *conf);

void
daemon_conf_deinit(struct daemon_conf *conf);

bool
daemon_conf_parse(struct daemon_conf *conf,
	FILE *filep);

/* DAEMON_CONF_H */
#endif
