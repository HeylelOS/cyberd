#include "daemon_conf.h"
#include "log.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <wordexp.h>
#include <errno.h>

/**
 * Section in the configuration file
 */
enum daemon_conf_section {
	SECTION_UNKNOWN,
	SECTION_GENERAL,
	SECTION_START
};

/**
 * Resolves a user name to a user uid
 * @param user User name or integral id of the desired uid
 * @param uidp Return value of the uid if the call is successfull
 * @return 0 on success -1 else
 */
static int
daemon_conf_parse_general_uid(const char *user, uid_t *uidp) {
	struct passwd *pwd;

	if((errno = 0, pwd = getpwnam(user)) == NULL) {
		if(errno == 0) {
			/* No entry found, treat as numeric gid */
			char *end;
			unsigned long luid = strtoul(user, &end, 10);

			if(*user == '\0' || *end != '\0') {
				log_print("Unable to infer uid from '%s'", user);
				return -1;
			} else {
				*uidp = (uid_t) luid;
			}
		} else {
			log_error("Unable to infer uid from '%s', getpwnam", user);
			return -1;
		}
	} else {
		*uidp = pwd->pw_uid;
	}

	return 0;
}

/**
 * Resolves a group name to a group gid
 * @param user Group name or integral id of the desired gid
 * @param gidp Return value of the gid if the call is successfull
 * @return 0 on success -1 else
 */
static int
daemon_conf_parse_general_gid(const char *group, gid_t *gidp) {
	struct group *grp;

	if((errno = 0, grp = getgrnam(group)) == NULL) {
		if(errno == 0) {
			/* No entry found, treat as numeric gid */
			char *end;
			unsigned long lgid = strtoul(group, &end, 10);

			if(*group == '\0' || *end != '\0') {
				log_print("Unable to infer gid from '%s'", group);
				return -1;
			} else {
				*gidp = (gid_t) lgid;
			}
		} else {
			log_error("Unable to infer gid from '%s', getgrnam", group);
			return -1;
		}
	} else {
		*gidp = grp->gr_gid;
	}

	return 0;
}

/**
 * Performs a shell expansion on a line of arguments
 * @param args Line of arguments to expand
 * @return List of arguments, NULL terminated
 */
static char **
daemon_conf_parse_general_expand_arguments(const char *args) {
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
			log_print("daemon_conf_parse: Argument list must have at least one argument");
		}

		wordfree(&p);
	}

	return arguments;
}

/**
 * Parses an element of the general section
 * @param conf Configuration being parsed
 * @key Key of the parameters value
 * @value Value associated to key, can be NULL
 * @return 0 on success or unknown key, -1 else
 */
static int
daemon_conf_parse_general(struct daemon_conf *conf,
	const char *key, const char *value) {

	if(value != NULL) {
		if(strcmp(key, "path") == 0) {
			free(conf->path);
			conf->path = strdup(value);
		} else if(strcmp(key, "user") == 0) {
			if(daemon_conf_parse_general_uid(value, &conf->uid) == -1) {
				return -1;
			}
		} else if(strcmp(key, "group") == 0) {
			if(daemon_conf_parse_general_gid(value, &conf->gid) == -1) {
				return -1;
			}
		} else if(strcmp(key, "arguments") == 0) {
			char **arguments = daemon_conf_parse_general_expand_arguments(value);

			if(arguments == NULL) {
				return -1;
			}

			free(conf->arguments);
			conf->arguments = arguments;
		}
	}

	return 0;
}

/**
 * Parses an element of the start section
 * @param conf Configuration being parsed
 * @key Key of the parameters value
 * @value Value associated to key, can be NULL
 * @return 0 on success or unknown key, -1 else
 */
static int
daemon_conf_parse_start(struct daemon_conf *conf,
	const char *key, const char *value) {

	if(value == NULL) {
		if(strcmp(key, "load") == 0) {
			conf->startmask |= DAEMON_START_LOAD;
		} else if(strcmp(key, "reload") == 0) {
			conf->startmask |= DAEMON_START_RELOAD;
		}
	}

	return 0;
}

void
daemon_conf_init(struct daemon_conf *conf) {

	conf->path = NULL;
	conf->arguments = NULL;

	conf->sigend = SIGTERM;
	conf->sigreload = SIGHUP;

	conf->uid = getuid();
	conf->gid = getgid();

	/* Zero'ing startmask */
	conf->startmask = 0;
}

void
daemon_conf_deinit(struct daemon_conf *conf) {

	free(conf->path);
	free(conf->arguments);
}

bool
daemon_conf_parse(struct daemon_conf *conf,
	FILE *filep) {
	enum daemon_conf_section section = SECTION_GENERAL;
	ssize_t length;
	char *line = NULL;
	size_t linecap = 0;
	int errors = 0;

	/* Reading lines */
	while((length = getline(&line, &linecap, filep)) != -1) {
		if(length != 0) {
			line[length - 1] = '\0';
		}

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

			switch(section) {
			case SECTION_GENERAL:
				if(daemon_conf_parse_general(conf, line, value) != 0) {
					errors++;
				}
				break;
			case SECTION_START:
				if(daemon_conf_parse_start(conf, line, value) != 0) {
					errors++;
				}
				break;
			default: /* SECTION_UNKNOWN */
				break;
			}
		}
	}

	free(line);

	return conf->path != NULL && errors == 0;
}

