/* SPDX-License-Identifier: BSD-3-Clause */
#include "configuration.h"

#include "daemon.h"
#include "tree.h"

#include <string.h> /* strcmp */
#include <syslog.h> /* syslog */
#include <unistd.h> /* close */
#include <dirent.h> /* opendir, ... */
#include <fcntl.h> /* openat */
#include <errno.h> /* errno */

/*******************
 * Daemons storage *
 *******************/

/**
 * Daemon's tree comparison function. Identifies by the daemon's name.
 * @param lhs Left hand side operand.
 * @param rhs Right hand side operand.
 * @returns The comparison between @p lhs and @p rhs names, see _strcmp(3)_.
 */
static int
daemons_compare(const tree_element_t *lhs, const tree_element_t *rhs) {
	const struct daemon * const ldaemon = lhs, * const rdaemon = rhs;
	return strcmp(ldaemon->name, rdaemon->name);
}

/**
 * Main daemon storage, where all daemon's structure are allocated/freed.
 * This storage is indexed using the daemon's names as identifiers.
 * Daemons stored in `src/cyberd/spawns.c` come from here and are thus bound to this tree's storage lifetime.
 */
static struct tree daemons = { .compare = daemons_compare };

/**
 * Helper function to remove a daemon according to its name.
 * @param daemons Tree using @ref daemons_compare as a comparison function.
 * @param name Name of the daemon to remove.
 * @returns Tree node holding the daemon associated with @p name, or _NULL_ if not found.
 */
static struct daemon *
daemons_remove_element(struct tree *daemons, const char *name) {
	const struct daemon element = { .name = (char *)name }; /* Cast is safe as daemons_compare doesn't modify name */
	return tree_remove(daemons, &element);
}

/**
 * Destroy a daemon element.
 * @param element Daemon.
 */
static void
daemons_destroy_element(tree_element_t *element) {
	daemon_destroy(element);
}

/**
 * This function finds the daemon named @p name.
 * @param name Daemon's identifier.
 * @returns The associated daemon, or NULL if not found.
 */
struct daemon *
configuration_find(const char *name) {
	const struct daemon element = { .name = (char *)name }; /* Cast is safe as daemons_compare doesn't modify name */
	return tree_find(&daemons, &element);
}

/*********************************
 * Daemons configuration loading *
 *********************************/

/**
 * Daemons configurations path, loaded once by @ref configuration_load.
 */
static const char *configuration_path;

/**
 * Load a new daemon's configuration.
 * This function is called during @ref configuration_load, and may start
 * a daemon, which requires all other subsystems to already be available.
 * @param name Name of the daemon.
 * @param filep Opened configuration file.
 */
static void
configuration_load_daemon(const char *name, FILE *filep) {
	struct daemon * const daemon = daemon_create(name);

	if (daemon == NULL) {
		return syslog(LOG_ERR, "Failure to create daemon '%s'", name);
	}

	if (daemon_conf_parse(&daemon->conf, filep) != 0) {
		return daemon_destroy(daemon);
	}

	tree_insert(&daemons, daemon);

	syslog(LOG_INFO, "'%s' loaded", daemon->name);

	if (daemon->conf.start.load) {
		daemon_start(daemon);
	}
}

/**
 * Load a new daemon's configuration, or reload it if already existing.
 * @param name Name of the daemon.
 * @param filep Opened configuration file.
 * @param olddaemons Old daemons tree, where previous daemons are stored.
 */
static void
configuration_reload_daemon(const char *name, FILE *filep, struct tree *olddaemons) {
	struct daemon * const daemon = daemons_remove_element(olddaemons, name);

	if (daemon == NULL) {
		return configuration_load_daemon(name, filep); /* New daemon. */
	}

	/* Reloading existing daemon. */
	struct daemon_conf newconf;
	daemon_conf_init(&newconf);

	if (daemon_conf_parse(&newconf, filep) == 0) {
		daemon_conf_deinit(&daemon->conf);
		daemon->conf = newconf;

		syslog(LOG_INFO, "'%s' reloaded", daemon->name);

		if (daemon->conf.start.reload) {
			daemon_start(daemon);
		}
	} else {
		daemon_conf_deinit(&newconf);
		syslog(LOG_ERR, "Unable to reload '%s'", daemon->name);
	}

	tree_insert(&daemons, daemon);
}

/**
 * Open a FILE from a directory
 * @param dirfd Directory file descriptor.
 * @param path Path to the file, from @p dirfd.
 * @returns A valid stream on success, _NULL_ else.
 */
static FILE *
configuration_fopenat(int dirfd, const char *path) {
	const int fd = openat(dirfd, path, O_RDONLY);
	FILE *filep;

	if (fd >= 0) {
		filep = fdopen(fd, "r");
		if (filep == NULL) {
			close(fd);
			syslog(LOG_ERR, "configuration fdopen '%s': %m", path);
		}
	} else {
		syslog(LOG_ERR, "configuration openat '%s': %m", path);
		filep = NULL;
	}

	return filep;
}

/**
 * Load initial configuration. Called on @ref setup.
 * This function may start daemons, and suppose all subsystems have been initialized.
 */
void
configuration_load(const char *path) {
	DIR *dirp;

	configuration_path = path;
	syslog(LOG_INFO, "configuration_load %s", path);

	dirp = opendir(configuration_path);
	if (dirp == NULL) {
		return syslog(LOG_ERR, "configuration_load opendir '%s': %m", configuration_path);
	}

	struct dirent *entry;
	while (errno = 0, entry = readdir(dirp), entry != NULL) {
		FILE *filep;

		if (*entry->d_name == '.') {
			continue;
		}

		filep = configuration_fopenat(dirfd(dirp), entry->d_name);
		if (filep == NULL) {
			continue;
		}

		configuration_load_daemon(entry->d_name, filep);
		fclose(filep);
	}

	if (errno != 0) {
		syslog(LOG_ERR, "configuration_load readdir: %m");
	}

	closedir(dirp);
}

/**
 * Reload configurations, and load new ones if available.
 */
void
configuration_reload(void) {
	DIR *dirp;

	syslog(LOG_INFO, "configuration_reload %s", configuration_path);

	dirp = opendir(configuration_path);
	if (dirp == NULL) {
		return syslog(LOG_ERR, "configuration_reload opendir '%s': %m", configuration_path);
	}

	struct tree olddaemons = daemons;
	daemons.root = NULL;

	struct dirent *entry;
	while (errno = 0, entry = readdir(dirp), entry != NULL) {
		FILE *filep;

		if (*entry->d_name == '.') {
			continue;
		}

		filep = configuration_fopenat(dirfd(dirp), entry->d_name);
		if (filep == NULL) {
			continue;
		}

		configuration_reload_daemon(entry->d_name, filep, &olddaemons);
		fclose(filep);
	}

	if (errno != 0) {
		syslog(LOG_ERR, "configuration_reload readdir: %m");
	}

	closedir(dirp);

	tree_mutate(&olddaemons, daemons_destroy_element);
	tree_deinit(&olddaemons);
}

#ifndef NDEBUG
/**
 * Frees all configuration data. Not used in release mode because it
 * only frees memory, which is not anymore a sensible resource at this point.
 */
void
configuration_cleanup(void) {
	tree_mutate(&daemons, daemons_destroy_element);
	tree_deinit(&daemons);
}
#endif
