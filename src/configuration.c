#include "configuration.h"
#include "daemon.h"
#include "log.h"
#include "tree.h"

#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <dirent.h>

static const char *configdirs[] = {
	"./tests"
};

/* Main storage for daemons */
static struct tree daemons;

static hash_t
daemons_hash_field(const tree_element_t *element) {
	const struct daemon *daemon = element;

	return daemon->namehash;
}

/* Iterate each file in configdirs to load daemonconfs */
static void
daemons_dir_iterate(void (*callback)(const struct dirent *, FILE *)) {
	const char **iterator = configdirs;
	const char ** const end = configdirs + sizeof (configdirs) / sizeof (char *);
	char pathbuf[MAXPATHLEN];

	for (; iterator != end; iterator += 1) {
		char *pathend = stpncpy(pathbuf, *iterator, sizeof (pathbuf));
		size_t namemax;
		DIR *dirp;
		const struct dirent *entry;

		/* Preparing path iteration */
		if (pathend >= pathbuf + sizeof (pathbuf) - 1) {
			log_error("[cyberd daemons_dir_iterate] Path buffer overflow\n");
			continue;
		}
		*pathend = '/';
		pathend += 1;
		namemax = pathbuf + sizeof (pathbuf) - 1 - pathend;

		/* Open load directory */
		if ((dirp = opendir(pathbuf)) == NULL) {
			log_print("[cyberd daemons_dir_iterate] Unable to open directory \"%s\"\n", pathbuf);
			continue;
		}

		/* Iterating load directory */
		while ((entry = readdir(dirp)) != NULL) {
			if ((entry->d_type == DT_REG
				|| entry->d_type == DT_LNK)
				&& entry->d_name[0] != '.') {
				FILE *filep;

				strncpy(pathend, entry->d_name, namemax);

				/* Open the current daemon config file */
				if ((filep = fopen(pathbuf, "r")) != NULL) {

					callback(entry, filep);
					fclose(filep);
				} else {
					log_error("[cyberd fopen]");
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

	if (daemon_conf_parse(&daemon->conf, filep)) {
		struct tree_node *node = tree_node_create(daemon);

		tree_insert(&daemons, node);

		log_print("Loaded '%s'\n", daemon->name);
	} else {
		daemon_destroy(daemon);
	}
}

void
configuration_init(void) {

	tree_init(&daemons, daemons_hash_field);
	daemons_dir_iterate(daemons_load);
}

static void
daemons_reload(const struct dirent *entry,
	FILE *filep) {

	log_print("Reload '%s'\n", entry->d_name);
}

void
configuration_reload(void) {

	daemons_dir_iterate(daemons_reload);
}

