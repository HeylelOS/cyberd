#ifndef DAEMON_CONF_H
#define DAEMON_CONF_H

#include <stdio.h>
#include <stdbool.h>
#include <spawn.h>

/* struct to contain configurations */
struct daemon_conf {
	char *path;
	char **arguments;

	posix_spawn_file_actions_t file_actions;
	posix_spawnattr_t attr;
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
