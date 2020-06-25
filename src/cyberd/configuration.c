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

static int
daemons_compare_function(const tree_element_t *lhs, const tree_element_t *rhs);

static hash_t
daemons_hash_function(const tree_element_t *element);

/* Main storage for daemons */
static struct tree daemons;

static const struct tree_class daemons_tree_class = {
	.compare_function = daemons_compare_function,
	.hash_function = daemons_hash_function,
};

static int
daemons_compare_function(const tree_element_t *lhs, const tree_element_t *rhs) {
	const struct daemon *ldaemon = lhs, *rdaemon = rhs;

	return strcmp(ldaemon->name, rdaemon->name);
}

static hash_t
daemons_hash_function(const tree_element_t *element) {
	const struct daemon *daemon = element;

	return daemon->namehash;
}

static void
daemons_preorder_cleanup(struct tree_node *node) {

	if (node != NULL) {
		struct daemon *daemon = node->element;

		daemon_destroy(daemon);

		daemons_preorder_cleanup(node->left);
		daemons_preorder_cleanup(node->right);
	}
}

static void
configuration_daemons_load(const char *name, FILE *filep) {
	struct daemon *daemon = daemon_create(name);

	if (daemon != NULL) {
		if (daemon_conf_parse(&daemon->conf, filep)) {
			struct tree_node *node = tree_node_create(daemon);

			if (node != NULL) {
				tree_insert(&daemons, node);

				log_print("'%s' loaded", daemon->name);

				if (daemon->conf.start.load == 1) {
					daemon_start(daemon);
				}
			}
		} else {
			daemon_destroy(daemon);
		}
	}
}

static struct tree_node *
configuration_daemon_remove_by_name(struct tree *daemons, const char *name, hash_t hash) {
	struct tree_node *removed = tree_remove_by_hash(daemons, hash);
	struct tree_node *node = removed;

	if (removed != NULL) {
		struct daemon *daemon = node->element;

		if (strcmp(daemon->name, name) != 0) {
			node = configuration_daemon_remove_by_name(daemons, name, hash);
			tree_insert(daemons, removed);
		}
	}

	return node;
}

static void
configuration_daemons_reload(const char *name, FILE *filep, struct tree *olddaemons) {
	struct tree_node *node
		= configuration_daemon_remove_by_name(olddaemons, name, hash_string(name));

	if (node == NULL) {
		/* New daemon */
		configuration_daemons_load(name, filep);
	} else {
		/* Reloading existing daemon */
		struct daemon *daemon = node->element;

		daemon_conf_deinit(&daemon->conf);
		daemon_conf_init(&daemon->conf);

		if (daemon_conf_parse(&daemon->conf, filep)) {

			tree_insert(&daemons, node);

			log_print("'%s' reloaded", daemon->name);

			if (daemon->conf.start.reload == 1) {
				daemon_start(daemon);
			}
		} else {
			log_error("configuration_reload: Couldn't reload '%s'", daemon->name);

			daemon_destroy(daemon);
			tree_node_destroy(node);
		}
	}
}

static FILE *
configuration_fopenat(int dirfd, const char *path) {
	int fd = openat(dirfd, path, O_RDONLY);
	FILE *filep = NULL;

	if (fd >= 0) {
		if ((filep = fdopen(fd, "r")) == NULL) {
			close(fd);
			log_error("configuration fdopen '%s': %m", path);
		}
	} else {
		log_error("configuration openat '%s': %m", path);
	}

	return filep;
}

void
configuration_init(void) {
	DIR *dirp;

	log_print("configuration_init");
	tree_init(&daemons, &daemons_tree_class);

	if ((dirp = opendir(CONFIG_DAEMONCONFS_DIRECTORY)) != NULL) {
		struct dirent *entry;

		while ((errno = 0, entry = readdir(dirp)) != NULL) {
			FILE *filep;

			if (entry->d_type == DT_REG && *entry->d_name != '.'
				&& (filep = configuration_fopenat(dirfd(dirp), entry->d_name)) != NULL) {

				configuration_daemons_load(entry->d_name, filep);
			}
		}

		if (errno != 0) {
			log_error("configuration_init readdir: %m");
		}
	} else {
		log_error("configuration_init opendir: %m");
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

	log_print("configuration_reload");
	scheduler_empty();
	tree_init(&daemons, &daemons_tree_class);

	if ((dirp = opendir(CONFIG_DAEMONCONFS_DIRECTORY)) != NULL) {
		struct dirent *entry;

		while ((errno = 0, entry = readdir(dirp)) != NULL) {
			FILE *filep;

			if (entry->d_type == DT_REG && *entry->d_name != '.'
				&& (filep = configuration_fopenat(dirfd(dirp), entry->d_name)) != NULL) {

				configuration_daemons_reload(entry->d_name, filep, &olddaemons);
			}
		}

		if (errno != 0) {
			log_error("configuration_reload readdir: %m");
		}
	} else {
		log_error("configuration_reload opendir: %m");
	}

	daemons_preorder_cleanup(olddaemons.root);
	tree_deinit(&olddaemons);
}

struct daemon *
configuration_daemon_find(hash_t namehash) {

	return tree_find_by_hash(&daemons, namehash);
}

