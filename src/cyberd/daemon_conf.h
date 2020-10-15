#ifndef DAEMON_CONF_H
#define DAEMON_CONF_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

/** Structure which contains configurations for a daemon */
struct daemon_conf {
	char *path; /**< Path of the executable file */
	char **arguments; /**< Command line arguments, including process name */
	char **environment; /**< Command line environment variables */
	char *wd; /**< Working directory of the process */

	int sigfinish; /**< Signal used to terminate the process, default SIGTERM */
	int sigreload; /**< Signal used to reload the process configuration, default SIGHUP */

	uid_t uid; /**< User-id the process wil be executed with */
	gid_t gid; /**< Group-id the process will be executed with */

	mode_t umask; /**< umask of the daemon */
	int priority; /**< Scheduling priority of the daemon */

	struct {
		unsigned load : 1;        /**< Must be started at init load */
		unsigned reload : 1;      /**< Must be started at init reload */
		unsigned exitsuccess : 1; /**< Must be started when it stopped successfully */
		unsigned exitfailure : 1; /**< Must be started when it stopped unsuccessfully */
		unsigned killed : 1;      /**< Must be started when it was stopped by a signal */
		unsigned dumped : 1;      /**< Must be started when it dumped core */
	} start; /**< Bitmask holding when a daemon may want to start */
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
