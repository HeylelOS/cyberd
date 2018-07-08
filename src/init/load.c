#include "log.h"
#include "load.h"
#include "init.h"

#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include <dirent.h>
#include <wordexp.h>

static char **
expand_arguments(const char *args) {
	wordexp_t p;
	char **arguments = NULL;

	if (wordexp(args, &p, WRDE_NOCMD | WRDE_UNDEF) == 0) {
		if (p.we_wordc >= 1) {
			size_t total = 0;
			int i;

			for (i = 0; i < p.we_wordc; ++i) {
				total += strlen(p.we_wordv[i]) + 1;
			}

			arguments = malloc((p.we_wordc + 1) * sizeof (char *) + total);
			char *ptr = (char *)(arguments + p.we_wordc + 1);

			for (i = 0; i < p.we_wordc; ++i) {
				arguments[i] = ptr;
				ptr = stpcpy(ptr, p.we_wordv[i]) + 1;
			}
			arguments[i] = NULL;
		} else {
			log_print("    [expand_arguments] Argument list must have at least one argument\n");
		}

		wordfree(&p);
	}

	return arguments;
}

#define SECTION_UNKNOWN	-1
#define SECTION_CONFIG	0
#define SECTION_STARTUP	1

int
configure(struct daemon *daemon,
	FILE *filep) {
	int section = SECTION_CONFIG;
	ssize_t length;
	char *line = NULL;
	size_t linecap = 0;
	bool hasfile = false;

	log_print("[configure %s]\n", daemon->name);

	/* Reading current line */
	while ((length = getline(&line, &linecap, filep)) != -1) {
		line[length - 1] = '\0';

		/* Reading beginning of section */
		if (*line == '@') {

			if (strcmp(line, "@config") == 0) {
				section = SECTION_CONFIG;
			} else if (strcmp(line, "@startup") == 0) {
				section = SECTION_STARTUP;
			} else {
				section = SECTION_UNKNOWN;
			}
		} else if (*line != '\0'
			&& *line != '#') {
			char *value = line;

			strsep(&value, "=");

			switch (section) {
			case SECTION_CONFIG:
				if (value == NULL) {
					/* log_print("Config: \"%s\" is set\n", line); */
				} else {

					if (strcmp(line, "file") == 0) {

						free(daemon->file);
						daemon->file = strdup(value);
						hasfile = true;
					} else if (strcmp(line, "arguments") == 0) {

						free(daemon->arguments);
						daemon->arguments = expand_arguments(value);
					}
				}
				break;
			case SECTION_STARTUP:
				if (value == NULL) {

					if (strcmp(line, "launch") == 0) {
						struct scheduler_activity launch = {
							.daemon = daemon,
							.when = SCHEDULING_NOW,
							.action = SCHEDULE_START
						};

						scheduler_schedule(&init.scheduler, &launch);
					}
				} else {
					/* log_print("Start: \"%s\"=\"%s\"\n", line, sep); */
				}
				break;
			default:
				break;
			}
		}
	}

	free(line);

	if (hasfile) {
		return 0;
	} else {
		return -1;
	}
}

void
load(const char *directory) {
	char pathbuf[MAXPATHLEN];
	char *pathend = stpncpy(pathbuf, directory, sizeof (pathbuf));
	size_t namemax;
	DIR *dirp;
	struct dirent *entry;

	/* Preparing path iteration */
	if (pathend >= pathbuf + sizeof (pathbuf) - 1) {
		log_error("[daemons_init] Path buffer overflow\n");
		exit(EXIT_FAILURE);
	}
	*pathend = '/';
	pathend += 1;
	namemax = pathbuf + sizeof (pathbuf) - 1 - pathend;

	/* Open load directory */
	dirp = opendir(pathbuf);
	if (dirp == NULL) {
		log_print("[daemons_init] Unable to open directory \"%s\"\n", pathbuf);
		exit(EXIT_FAILURE);
	}

	/* Iterating load directory */
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_type == DT_REG
			|| entry->d_type == DT_LNK) {
			FILE *filep;

			strncpy(pathend, entry->d_name, namemax);

			/* Open the current daemon config file */
			if ((filep = fopen(pathbuf, "r")) != NULL) {
				struct daemons_node *node = daemons_node_create(entry->d_name);

				if (configure(&node->daemon, filep) == 0) {
					daemons_insert(&init.daemons, node);
				} else {
					daemons_node_destroy(node);
				}

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

