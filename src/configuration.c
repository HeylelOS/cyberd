#include "configuration.h"
#include "daemon.h"
#include "log.h"
#include "tree.h"

#include "../config.h" /* configurationdirs */

#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <dirent.h>
#include <errno.h> /* to show overflow in daemons_dir_iterate */

/* Main storage for daemons */
static struct tree daemons;
/* Temporary tree used when reloading configuration */
static struct tree reloadtmp;

static hash_t
daemons_hash_field(const tree_element_t *element) {
	const struct daemon *daemon = element;

	return daemon->namehash;
}

/* Iterate each file in configurationdirs to load daemonconfs */
static void
daemons_dir_iterate(void (*callback)(const struct dirent *, FILE *)) {
	const char **iterator = configurationdirs;
	const char ** const end
		= configurationdirs + sizeof(configurationdirs) / sizeof(char *);
	char pathbuf[MAXPATHLEN];

	for(; iterator != end; iterator += 1) {
		char *pathend = stpncpy(pathbuf, *iterator, sizeof(pathbuf));
		size_t namemax;
		DIR *dirp;
		const struct dirent *entry;

		/* Preparing path iteration */
		if(pathend >= pathbuf + sizeof(pathbuf) - 1) {
			errno = EOVERFLOW;
			log_error("daemons_dir_iterate");
			continue;
		}
		*pathend = '/';
		pathend += 1;
		namemax = pathbuf + sizeof(pathbuf) - 1 - pathend;

		/* Open load directory */
		if((dirp = opendir(pathbuf)) == NULL) {
			log_print("daemons_dir_iterate: Unable to open configuration directory \"%s\"\n",
				pathbuf);
			continue;
		}

		log_print("Iterating configuration directory '%s'\n", pathbuf);

		/* Iterating load directory */
		while((entry = readdir(dirp)) != NULL) {
			if((entry->d_type == DT_REG
				|| entry->d_type == DT_LNK)
				&& entry->d_name[0] != '.') {
				FILE *filep;

				strncpy(pathend, entry->d_name, namemax);

				/* Open the current daemon config file */
				if((filep = fopen(pathbuf, "r")) != NULL) {

					callback(entry, filep);
					fclose(filep);
				} else {
					log_error("fopen");
				}

				/* Closing path */
				*pathend = '\0';
			}
		}

		closedir(dirp);
	}
}

static void
daemons_load(const struct dirent *entry,
	FILE *filep) {
	struct daemon *daemon = daemon_create(entry->d_name);

	if(daemon_conf_parse(&daemon->conf, filep)) {
		struct tree_node *node = tree_node_create(daemon);

		tree_insert(&daemons, node);

		log_print("Loaded '%s'\n", daemon->name);

		if(DAEMON_START_AT(daemon, DAEMON_START_LOAD)) {
			daemon_start(daemon);
		}
	} else {
		daemon_destroy(daemon);
	}
}

void
configuration_init(void) {

	log_print("Configurating...");
	tree_init(&daemons, daemons_hash_field);
	daemons_dir_iterate(daemons_load);
}

static void
daemons_reload(const struct dirent *entry,
	FILE *filep) {
	struct tree_node *node
		= tree_remove(&reloadtmp, hash_string(entry->d_name));

	if(node == NULL) {
		/* New daemon */
		daemons_load(entry, filep);
	} else {
		/* Reloading existing daemon */
		struct daemon *daemon = node->element;

		daemon_conf_deinit(&daemon->conf);
		daemon_conf_init(&daemon->conf);

		if(daemon_conf_parse(&daemon->conf, filep)) {

			tree_insert(&daemons, node);

			log_print("Reloaded '%s'\n", daemon->name);

			if(DAEMON_START_AT(daemon, DAEMON_START_RELOAD)) {
				daemon_start(daemon);
			}
		} else {
			log_print("Error while reloading '%s'\n", daemon->name);

			daemon_destroy(daemon);
			tree_node_destroy(node);
		}
	}
}

static void
daemons_preorder_cleanup(struct tree_node *node) {

	if(node != NULL) {
		struct daemon *daemon = node->element;

		daemon_destroy(daemon);

		daemons_preorder_cleanup(node->left);
		daemons_preorder_cleanup(node->right);
	}
}

void
configuration_reload(void) {

	log_print("Reconfigurating...");
	reloadtmp = daemons;
	tree_init(&daemons, daemons_hash_field);
	daemons_dir_iterate(daemons_reload);
	daemons_preorder_cleanup(reloadtmp.root);
	tree_deinit(&reloadtmp);
}

