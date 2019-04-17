#include "../daemon_conf.h"
#include "../log.h"

#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

/**
 * Performs a shell expansion on a line of arguments
 * @param args Line of arguments to expand
 * @return List of arguments, NULL terminated
 */
static char **
daemon_conf_expand_arguments(const char *args) {
	wordexp_t p;
	char **arguments = NULL;

	if(wordexp(args, &p, WRDE_NOCMD | WRDE_UNDEF) == 0) {
		if(p.we_wordc >= 1) {
			size_t total = 0;
			int i;
			char *str;

			for(i = 0; i < p.we_wordc; i += 1) {
				total += strlen(p.we_wordv[i]) + 1;
			}

			arguments = malloc((p.we_wordc + 1) * sizeof(char *) + total);
			str = (char *)(arguments + p.we_wordc + 1);

			for(i = 0; i < p.we_wordc; i += 1) {
				arguments[i] = str;
				str = stpcpy(str, p.we_wordv[i]) + 1;
			}
			arguments[i] = NULL;
		} else {
			log_print("    [daemon_conf_parse] Argument list must have at least one argument\n");
		}

		wordfree(&p);
	}

	return arguments;
}

/**
 * Section in the configuration file
 */
enum daemon_conf_section {
	SECTION_UNKNOWN,
	SECTION_GENERAL,
	SECTION_START
};

bool
daemon_conf_parse(struct daemon_conf *conf,
	FILE *filep) {
	enum daemon_conf_section section = SECTION_GENERAL;
	ssize_t length;
	char *line = NULL;
	size_t linecap = 0;

	/* Reading lines */
	while((length = getline(&line, &linecap, filep)) != -1) {
		line[length - 1] = '\0';

		/* Reading beginning of section */
		if(*line == '@') {

			if(strcmp(line, "@general") == 0) {
				section = SECTION_GENERAL;
			} else if(strcmp(line, "@start") == 0) {
				section = SECTION_START;
			} else {
				section = SECTION_UNKNOWN;
			}
		} else if(*line != '\0'
			&& *line != '#') {
			char *value = line;

			strsep(&value, "=");

			/* This function will certainly more ugly in the future */
			switch(section) {
			case SECTION_GENERAL:
				if(value == NULL) {
				} else {

					if(strcmp(line, "path") == 0) {

						free(conf->path);
						conf->path = strdup(value);
					} else if(strcmp(line, "arguments") == 0) {

						free(conf->arguments);
						conf->arguments = daemon_conf_expand_arguments(value);
					}
				}
				break;
			case SECTION_START:
				if(value == NULL) {

					if(strcmp(line, "load") == 0) {
						conf->startmask |= DAEMON_START_LOAD;
					} else if(strcmp(line, "reload") == 0) {
						conf->startmask |= DAEMON_START_RELOAD;
					}
				} else {
				}
				break;
			default:
				break;
			}
		}
	}

	free(line);

	return conf->path != NULL;
}

