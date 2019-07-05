#include "configuration.h"
#include "log.h"
#include "tree.h"
#include "scheduler.h"

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

/* Main storage for daemons */
static struct tree daemons;

static hash_t
daemons_hash_field(const tree_element_t *element) {
	const struct daemon *daemon = element;

	return daemon->namehash;
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

static void
configuration_daemons_load(const char *name, FILE *filep) {
	struct daemon *daemon = daemon_create(name);

	if(daemon_conf_parse(&daemon->conf, filep)) {
		struct tree_node *node = tree_node_create(daemon);

		tree_insert(&daemons, node);

		log_print("Loaded '%s'\n", daemon->name);

		if(DAEMON_STARTS_AT(daemon, DAEMON_START_LOAD)) {
			daemon_start(daemon);
		}
	} else {
		daemon_destroy(daemon);
	}
}

static void
configuration_daemons_reload(const char *name, FILE *filep, struct tree *olddaemons) {
	struct tree_node *node
		= tree_remove(olddaemons, hash_string(name));

	if(node == NULL) {
		/* New daemon */
		configuration_daemons_load(name, filep);
	} else {
		/* Reloading existing daemon */
		struct daemon *daemon = node->element;

		daemon_conf_deinit(&daemon->conf);
		daemon_conf_init(&daemon->conf);

		if(daemon_conf_parse(&daemon->conf, filep)) {

			tree_insert(&daemons, node);

			log_print("Reloaded '%s'\n", daemon->name);

			if(DAEMON_STARTS_AT(daemon, DAEMON_START_RELOAD)) {
				daemon_start(daemon);
			}
		} else {
			log_print("Error while reloading '%s'\n", daemon->name);

			daemon_destroy(daemon);
			tree_node_destroy(node);
		}
	}
}

static FILE *
configuration_fopenat(int dirfd, const char *path) {
	int fd = openat(dirfd, path, O_RDONLY);
	FILE *filep = NULL;

	if(fd >= 0) {
		if((filep = fdopen(fd, "r")) == NULL) {
			close(fd);
			log_error("fdopen");
		}
	} else {
		log_error("openat");
	}

	return filep;
}

void
configuration_init(void) {
	DIR *dirp;

	log_print("Configurating...\n");
	tree_init(&daemons, daemons_hash_field);

	if((dirp = opendir(CONFIG_DAEMONCONFS_DIRECTORY)) != NULL) {
		struct dirent *entry;

		while((errno = 0, entry = readdir(dirp)) != NULL) {
			FILE *filep;

			if(entry->d_type == DT_REG && *entry->d_name != '.'
				&& (filep = configuration_fopenat(dirfd(dirp), entry->d_name)) != NULL) {

				configuration_daemons_load(entry->d_name, filep);
			}
		}

		if(errno != 0) {
			log_error("readdir");
		}
	} else {
		log_error("opendir");
	}
}

#ifdef CONFIG_FULL_CLEANUP
void
configuration_deinit(void) {

	daemons_preorder_cleanup(daemons.root);
	tree_deinit(&daemons);
}
#endif

void
configuration_reload(void) {
	struct tree olddaemons = daemons;
	DIR *dirp;

	log_print("Reconfigurating...");
	scheduler_empty();
	tree_init(&daemons, daemons_hash_field);

	if((dirp = opendir(CONFIG_DAEMONCONFS_DIRECTORY)) != NULL) {
		struct dirent *entry;

		while((errno = 0, entry = readdir(dirp)) != NULL) {
			FILE *filep;

			if(entry->d_type == DT_REG && *entry->d_name != '.'
				&& (filep = configuration_fopenat(dirfd(dirp), entry->d_name)) != NULL) {

				configuration_daemons_reload(entry->d_name, filep, &olddaemons);
			}
		}

		if(errno != 0) {
			log_error("readdir");
		}
	} else {
		log_error("opendir");
	}

	daemons_preorder_cleanup(olddaemons.root);
	tree_deinit(&olddaemons);
}

struct daemon *
configuration_daemon_find(hash_t namehash) {

	return tree_find(&daemons, namehash);
}

