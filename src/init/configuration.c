#include "log.h"
#include "configuration.h"
#include "daemons/daemons.h"
#include "scheduler/scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include <dirent.h>
#include <wordexp.h>

static const char *configdirs[] = {
	"./tests"
};

static char **
expand_arguments(const char *args) {
	wordexp_t p;
	char **arguments = NULL;

	if (wordexp(args, &p, WRDE_NOCMD | WRDE_UNDEF) == 0) {
		if (p.we_wordc >= 1) {
			size_t total = 0;
			int i;
			char *str;

			for (i = 0; i < p.we_wordc; ++i) {
				total += strlen(p.we_wordv[i]) + 1;
			}

			arguments = malloc((p.we_wordc + 1) * sizeof (char *) + total);
			str = (char *)(arguments + p.we_wordc + 1);

			for (i = 0; i < p.we_wordc; ++i) {
				arguments[i] = str;
				str = stpcpy(str, p.we_wordv[i]) + 1;
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

static int
configure(struct daemon *daemon,
	FILE *filep) {
	int section = SECTION_CONFIG;
	ssize_t length;
	char *line = NULL;
	size_t linecap = 0;

	log_print("[configure %s]\n", daemon->name);

	/* Reading lines */
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
					} else if (strcmp(line, "arguments") == 0) {

						free(daemon->arguments);
						daemon->arguments = expand_arguments(value);
					}
				}
				break;
			case SECTION_STARTUP:
				if (value == NULL) {

					if (strcmp(line, "launch") == 0) {
						extern struct scheduler scheduler;
						struct scheduler_activity launch = {
							.daemon = daemon,
							.when = SCHEDULING_ASAP,
							.action = SCHEDULE_START
						};

						scheduler_schedule(&scheduler, &launch);
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

	if (daemon->file != NULL) {
		return 0;
	} else {
		return -1;
	}
}

void
configuration(int loadtype) {
	const char **iterator = configdirs;
	const char **end = configdirs + sizeof (configdirs) / sizeof (char *);
	char pathbuf[MAXPATHLEN];

	for (; iterator != end; ++iterator) {
		char *pathend = stpncpy(pathbuf, *iterator, sizeof (pathbuf));
		size_t namemax;
		DIR *dirp;
		struct dirent *entry;

		/* Preparing path iteration */
		if (pathend >= pathbuf + sizeof (pathbuf) - 1) {
			log_error("[daemons_init] Path buffer overflow\n");
			continue;
		}
		*pathend = '/';
		pathend += 1;
		namemax = pathbuf + sizeof (pathbuf) - 1 - pathend;

		/* Open load directory */
		dirp = opendir(pathbuf);
		if (dirp == NULL) {
			log_print("[daemons_init] Unable to open directory \"%s\"\n", pathbuf);
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

					switch (loadtype) {
					case CONFIG_LOAD: {
						struct daemons_node *node = daemons_node_create(entry->d_name);

						if (configure(&node->daemon, filep) == 0) {
							extern struct daemons daemons;
							daemons_insert(&daemons, node);
						} else {
							daemons_node_destroy(node);
						}
					} break;
					case CONFIG_RELOAD: {
						log_print("Reloading configuration for \"%s\"...\n", entry->d_name);
					} break;
					default:
						break;
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
}

