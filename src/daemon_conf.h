#ifndef DAEMON_CONF_H
#define DAEMON_CONF_H

#include <stdio.h>
#include <stdbool.h>
#include <spawn.h>

/* struct to contain configurations */
struct daemon_conf {
	char *path;
	char **arguments;
	int sigend; /* Signal used to end the process, default SIGTERM */
	int sigreload; /* Signal used to reload the process configuration, default SIGHUP */

	posix_spawn_file_actions_t file_actions;
	posix_spawnattr_t attr;

#define DAEMON_START_AT(d, m) (((d)->conf.startmask & (m)) != 0)
#define DAEMON_START_LOAD (1 << 0)
#define DAEMON_START_RELOAD (1 << 1)

	int startmask;
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
