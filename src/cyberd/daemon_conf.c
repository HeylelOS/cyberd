#include "daemon_conf.h"
#include "log.h"

#include "config.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <wordexp.h>
#include <ctype.h>
#include <errno.h>

/**
 * Section in the configuration file
 */
enum daemon_conf_section {
	SECTION_GENERAL,
	SECTION_ENVIRONMENT,
	SECTION_START,
	SECTION_UNKNOWN,
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

	if ((errno = 0, pwd = getpwnam(user)) == NULL) {
		if (errno == 0) {
			/* No entry found, treat as numeric gid */
			char *end;
			unsigned long luid = strtoul(user, &end, 10);

			if (*user == '\0' || *end != '\0') {
				log_error("Unable to infer uid from '%s'", user);
				return -1;
			} else {
				*uidp = (uid_t) luid;
			}
		} else {
			log_error("Unable to infer uid from '%s', getpwnam: %m", user);
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

	if ((errno = 0, grp = getgrnam(group)) == NULL) {
		if (errno == 0) {
			/* No entry found, treat as numeric gid */
			char *end;
			unsigned long lgid = strtoul(group, &end, 10);

			if (*group == '\0' || *end != '\0') {
				log_error("Unable to infer gid from '%s'", group);
				return -1;
			} else {
				*gidp = (gid_t) lgid;
			}
		} else {
			log_error("Unable to infer gid from '%s', getgrnam: %m", group);
			return -1;
		}
	} else {
		*gidp = grp->gr_gid;
	}

	return 0;
}

static int
daemon_conf_parse_general_signal(const char *signalname, int *signalp) {

	if (isdigit(*signalname)) {
			char *end;
			unsigned long lsignal = strtoul(signalname, &end, 10);

			if (*end == '\0' && lsignal <= (int)-1) {
				log_error("Unable to infer decimal signal from '%s'", signalname);
				return -1;
			} else {
				*signalp = (int) lsignal;
			}
	} else if (strcmp("SIGABRT", signalname) == 0) {
		*signalp = SIGABRT;
	} else if (strcmp("SIGALRM", signalname) == 0) {
		*signalp = SIGALRM;
	} else if (strcmp("SIGBUS", signalname) == 0) {
		*signalp = SIGBUS;
	} else if (strcmp("SIGCHLD", signalname) == 0) {
		*signalp = SIGCHLD;
	} else if (strcmp("SIGCONT", signalname) == 0) {
		*signalp = SIGCONT;
	} else if (strcmp("SIGFPE", signalname) == 0) {
		*signalp = SIGFPE;
	} else if (strcmp("SIGHUP", signalname) == 0) {
		*signalp = SIGHUP;
	} else if (strcmp("SIGILL", signalname) == 0) {
		*signalp = SIGILL;
	} else if (strcmp("SIGINT", signalname) == 0) {
		*signalp = SIGINT;
	} else if (strcmp("SIGKILL", signalname) == 0) {
		*signalp = SIGKILL;
	} else if (strcmp("SIGPIPE", signalname) == 0) {
		*signalp = SIGPIPE;
	} else if (strcmp("SIGQUIT", signalname) == 0) {
		*signalp = SIGQUIT;
	} else if (strcmp("SIGSEGV", signalname) == 0) {
		*signalp = SIGSEGV;
	} else if (strcmp("SIGSTOP", signalname) == 0) {
		*signalp = SIGSTOP;
	} else if (strcmp("SIGTERM", signalname) == 0) {
		*signalp = SIGTERM;
	} else if (strcmp("SIGTSTP", signalname) == 0) {
		*signalp = SIGTSTP;
	} else if (strcmp("SIGTTIN", signalname) == 0) {
		*signalp = SIGTTIN;
	} else if (strcmp("SIGTTOU", signalname) == 0) {
		*signalp = SIGTTOU;
	} else if (strcmp("SIGUSR1", signalname) == 0) {
		*signalp = SIGUSR1;
	} else if (strcmp("SIGUSR2", signalname) == 0) {
		*signalp = SIGUSR2;
	} else if (strcmp("SIGPOLL", signalname) == 0) {
		*signalp = SIGPOLL;
	} else if (strcmp("SIGPROF", signalname) == 0) {
		*signalp = SIGPROF;
	} else if (strcmp("SIGSYS", signalname) == 0) {
		*signalp = SIGSYS;
	} else if (strcmp("SIGTRAP", signalname) == 0) {
		*signalp = SIGTRAP;
	} else if (strcmp("SIGURG", signalname) == 0) {
		*signalp = SIGURG;
	} else if (strcmp("SIGVTALRM", signalname) == 0) {
		*signalp = SIGVTALRM;
	} else if (strcmp("SIGXCPU", signalname) == 0) {
		*signalp = SIGXCPU;
	} else if (strcmp("SIGXFSZ", signalname) == 0) {
		*signalp = SIGXFSZ;
	} else {
		log_error("Unable to infer signal from '%s'", signalname);
		return -1;
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

	if (wordexp(args, &p, WRDE_NOCMD | WRDE_UNDEF) == 0) {
		if (p.we_wordc >= 1) {
			size_t total = 0;
			int i;

			for (i = 0; i < p.we_wordc; i += 1) {
				total += strlen(p.we_wordv[i]) + 1;
			}

			arguments = malloc((p.we_wordc + 1) * sizeof (char *) + total);

			if (arguments != NULL) {
				char *str = (char *)(arguments + p.we_wordc + 1);

				for (i = 0; i < p.we_wordc; i += 1) {
					arguments[i] = str;
					str = stpcpy(str, p.we_wordv[i]) + 1;
				}
				arguments[i] = NULL;
			}
		} else {
			log_error("daemon_conf_parse: Argument list must have at least one argument");
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

	if (value != NULL) {
		if (strcmp(key, "path") == 0) {
			char *newpath = strdup(value);

			if (newpath != NULL) {
				free(conf->path);
				conf->path = newpath;
			} else {
				return -1;
			}
		} else if (strcmp(key, "workdir") == 0) {
			char *newwd;

			if (*value == '/' && (newwd = strdup(value)) != NULL) {
				free(conf->wd);
				conf->wd = newwd;
			} else {
				return -1;
			}
		} else if (strcmp(key, "user") == 0) {
			if (daemon_conf_parse_general_uid(value, &conf->uid) == -1) {
				return -1;
			}
		} else if (strcmp(key, "group") == 0) {
			if (daemon_conf_parse_general_gid(value, &conf->gid) == -1) {
				return -1;
			}
		} else if (strcmp(key, "umask") == 0) {
			char *valueend;
			unsigned long cmask = strtoul(value, &valueend, 8);

			if (*value == '\0' || *valueend != '\0') {
				return -1;
			}

			conf->umask = cmask & 0x1FF;

		} else if (strcmp(key, "sigend") == 0) {
			if (daemon_conf_parse_general_signal(value, &conf->sigend) == -1) {
				return -1;
			}
		} else if (strcmp(key, "sigreload") == 0) {
			if (daemon_conf_parse_general_signal(value, &conf->sigreload) == -1) {
				return -1;
			}
		} else if (strcmp(key, "arguments") == 0) {
			char **arguments = daemon_conf_parse_general_expand_arguments(value);

			if (arguments == NULL) {
				return -1;
			}

			free(conf->arguments);
			conf->arguments = arguments;
		}
	}

	return 0;
}

/**
 * Adds an element to the daemon environment
 * @param conf Configuration being parsed
 * @param string String of the form <key>=<value>
 * @param envcapp In-Out value correspondig to the capacity of conf->environment
 * @return 0 on success or unknown key, -1 else
 */
static int
daemon_conf_parse_environment(struct daemon_conf *conf, const char *string, size_t *envcapp) {

	if (strchr(string, '=') != NULL) {
		size_t end;

		if (conf->environment == NULL) {
			end = 0;
			*envcapp = 16;
			conf->environment = malloc(sizeof (*conf->environment) * *envcapp);

			if (conf->environment == NULL) {
				*envcapp = 0;
				return -1;
			}
		} else {
			char **env = conf->environment;
			while (*env != NULL) {
				env++;
			}
			end = env - conf->environment;

			if (*envcapp == end) {
				char **newenvironment = realloc(conf->environment, sizeof (*conf->environment) * *envcapp * 2);

				if (newenvironment == NULL) {
					return -1;
				}

				conf->environment = newenvironment;
				*envcapp *= 2;
			}
		}

		if ((conf->environment[end] = strdup(string)) == NULL) {
			return -1;
		}
		conf->environment[end + 1] = NULL;

		return 0;
	} else {
		log_error("daemon_conf_parse: Environment variable must be of type <key>=<value>, found '%s'", string);
	}

	return -1;
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

	if (value == NULL) {
		if (strcmp(key, "load") == 0) {
			conf->startmask |= DAEMON_START_LOAD;
		} else if (strcmp(key, "reload") == 0) {
			conf->startmask |= DAEMON_START_RELOAD;
		}
	}

	return 0;
}

void
daemon_conf_init(struct daemon_conf *conf) {

	conf->path = NULL;
	conf->arguments = NULL;
	conf->environment = NULL;
	conf->wd = NULL;

	conf->sigend = SIGTERM;
	conf->sigreload = SIGHUP;

	conf->uid = getuid();
	conf->gid = getgid();

	conf->umask = CONFIG_DEFAULT_UMASK;

	/* Zero'ing startmask */
	conf->startmask = 0;
}

void
daemon_conf_deinit(struct daemon_conf *conf) {

	free(conf->path);
	free(conf->arguments);
	if (conf->environment != NULL) {
		char **env = conf->environment;

		while (*env != NULL) {
			free(*env);
			env++;
		}

		free(conf->environment);
	}
	free(conf->wd);
}

bool
daemon_conf_parse(struct daemon_conf *conf,
	FILE *filep) {
	enum daemon_conf_section section = SECTION_GENERAL;
	char *line = NULL;
	size_t linecap = 0, envcap = 0;
	int errors = 0;
	ssize_t length;

	/* Reading lines */
	while ((length = getline(&line, &linecap, filep)) != -1) {
		if (length != 0) {
			line[length - 1] = '\0';
		}

		/* Reading beginning of section */
		if (*line == '@') {

			if (strcmp(line, "@general") == 0) {
				section = SECTION_GENERAL;
			} else if (strcmp(line, "@environment") == 0) {
				section = SECTION_ENVIRONMENT;
			} else if (strcmp(line, "@start") == 0) {
				section = SECTION_START;
			} else {
				section = SECTION_UNKNOWN;
			}
		} else if (*line != '\0'
			&& *line != '#') {
			char *value = line;

			switch (section) {
			case SECTION_GENERAL:
				if (daemon_conf_parse_general(conf, strsep(&value, "="), value) != 0) {
					errors++;
				}
				break;
			case SECTION_ENVIRONMENT:
				if (daemon_conf_parse_environment(conf, value, &envcap) != 0) {
					errors++;
				}
			case SECTION_START:
				if (daemon_conf_parse_start(conf, strsep(&value, "="), value) != 0) {
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

