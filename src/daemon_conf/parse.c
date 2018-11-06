#include "../daemon_conf.h"
#include "../log.h"

#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

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

#define SECTION_UNKNOWN	-1
#define SECTION_CONFIG	0
#define SECTION_STARTUP	1

bool
daemon_conf_parse(struct daemon_conf *conf,
	FILE *filep) {
	int section = SECTION_CONFIG;
	ssize_t length;
	char *line = NULL;
	size_t linecap = 0;

	/* Reading lines */
	while((length = getline(&line, &linecap, filep)) != -1) {
		line[length - 1] = '\0';

		/* Reading beginning of section */
		if(*line == '@') {

			if(strcmp(line, "@config") == 0) {
				section = SECTION_CONFIG;
			} else if(strcmp(line, "@startup") == 0) {
				section = SECTION_STARTUP;
			} else {
				section = SECTION_UNKNOWN;
			}
		} else if(*line != '\0'
			&& *line != '#') {
			char *value = line;

			strsep(&value, "=");

			switch(section) {
			case SECTION_CONFIG:
				if(value == NULL) {
					/* log_print("Config: \"%s\" is set\n", line); */
				} else {

					if(strcmp(line, "file") == 0) {

						free(conf->file);
						conf->file = strdup(value);
					} else if(strcmp(line, "arguments") == 0) {

						free(conf->arguments);
						conf->arguments = daemon_conf_expand_arguments(value);
					}
				}
				break;
			case SECTION_STARTUP:
				if(value == NULL) {

					if(strcmp(line, "launch") == 0) {
						/* Shall immediately launch the daemon */
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

	return conf->file != NULL;
}

