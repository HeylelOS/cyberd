#ifndef DAEMON_CONF_H
#define DAEMON_CONF_H

#include <stdio.h>
#include <stdbool.h>
#include <spawn.h>

/** Structure which contains configurations for a daemon */
struct daemon_conf {
	char *path; /**< Path of the executable file */
	char **arguments; /**< Command line arguments, including process name */
	int sigend; /**< Signal used to stop the process, default SIGTERM */
	int sigreload; /**< Signal used to reload the process configuration, default SIGHUP */

	posix_spawn_file_actions_t file_actions;
	posix_spawnattr_t attr;

#define DAEMON_STARTS_AT(d, m) (((d)->conf.startmask & (m)) != 0)
#define DAEMON_START_LOAD (1 << 0)
#define DAEMON_START_RELOAD (1 << 1)

	int startmask; /**< Bitmask holding when a daemon may want to start */
};

/**
 * Initializes a daemon_conf for later parsing
 * @param conf Configuration to initialize
 */
void
daemon_conf_init(struct daemon_conf *conf);

/**
 * Deinitializes a daemon_conf
 * @param conf daemon_conf to deinitialize
 */
void
daemon_conf_deinit(struct daemon_conf *conf);

/**
 * Tries parsing a daemon_conf from a file.
 * @param conf daemon_conf to parse
 * @param filep File to read and parse from
 * @return Whether it was successfull
 */
bool
daemon_conf_parse(struct daemon_conf *conf,
	FILE *filep);

/* DAEMON_CONF_H */
#endif
