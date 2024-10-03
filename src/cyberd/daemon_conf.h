/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef DAEMON_CONF_H
#define DAEMON_CONF_H

#include <stdio.h> /* FILE */
#include <sys/types.h> /* uid_t, gid_t, mask_t */

/** Structure which contains configurations for a daemon */
struct daemon_conf {
	char *path; /**< Path of the executable file */
	char **arguments; /**< Command line arguments, including process name */
	char **environment; /**< Command line environment variables */
	char *workdir; /**< Working directory of the process */
	char *in; /**< Standard input of the process */
	char *out; /**< Standard output of the process */
	char *err; /**< Standard error of the process */

	int sigfinish; /**< Signal used to terminate the process, default SIGTERM */
	int sigreload; /**< Signal used to reload the process configuration, default SIGHUP */

	uid_t uid; /**< User-id the process wil be executed with */
	gid_t gid; /**< Group-id the process will be executed with */
	unsigned int nosid : 1; /**< Do not setsid when the process is forked */

	mode_t umask; /**< umask of the daemon */
	int priority; /**< Scheduling priority of the daemon */

	struct {
		unsigned int load : 1;        /**< Must be started at configuration load */
		unsigned int reload : 1;      /**< Must be started at configuration reload */
		unsigned int exitsuccess : 1; /**< Must be started when it stopped successfully */
		unsigned int exitfailure : 1; /**< Must be started when it stopped unsuccessfully */
		unsigned int killed : 1;      /**< Must be started when it was stopped by a signal */
		unsigned int dumped : 1;      /**< Must be started when it dumped core */
	} start; /**< Bitmask holding when a daemon wants to be started */
};

void
daemon_conf_init(struct daemon_conf *conf);

void
daemon_conf_deinit(struct daemon_conf *conf);

int
daemon_conf_parse(struct daemon_conf *conf, FILE *filep);

/* DAEMON_CONF_H */
#endif
