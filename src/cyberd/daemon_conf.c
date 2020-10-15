#include "daemon_conf.h"
#include "log.h"

#include "config.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <errno.h>

#define DAEMON_CONF_STRINGLIST_DEFAULT_CAPACITY 16

/**
 * Section in the configuration file
 */
enum daemon_conf_section {
	SECTION_GENERAL,
	SECTION_ENVIRONMENT,
	SECTION_START,
	SECTION_UNKNOWN,
};

static const struct signalpair {
	const int signum;
	const char signame[7];
} signals[] = {
	{ SIGABRT,   "ABRT"   },
	{ SIGALRM,   "ALRM"   },
	{ SIGBUS,    "BUS"    },
	{ SIGCHLD,   "CHLD"   },
	{ SIGCONT,   "CONT"   },
	{ SIGFPE,    "FPE"    },
	{ SIGHUP,    "HUP"    },
	{ SIGILL,    "ILL"    },
	{ SIGINT,    "INT"    },
	{ SIGKILL,   "KILL"   },
	{ SIGPIPE,   "PIPE"   },
	{ SIGQUIT,   "QUIT"   },
	{ SIGSEGV,   "SEGV"   },
	{ SIGSTOP,   "STOP"   },
	{ SIGTERM,   "TERM"   },
	{ SIGTSTP,   "TSTP"   },
	{ SIGTTIN,   "TTIN"   },
	{ SIGTTOU,   "TTOU"   },
	{ SIGUSR1,   "USR1"   },
	{ SIGUSR2,   "USR2"   },
#ifndef __APPLE__
	{ SIGPOLL,   "POLL"   },
#endif
	{ SIGPROF,   "PROF"   },
	{ SIGSYS,    "SYS"    },
	{ SIGTRAP,   "TRAP"   },
	{ SIGURG,    "URG"    },
	{ SIGVTALRM, "VTALRM" },
	{ SIGXCPU,   "XCPU"   },
	{ SIGXFSZ,   "XFSZ"   }
};

/**
 * Frees a NULL-terminated list of strings
 * @param stringlist List to deallocate, can be NULL
 */
static void
daemon_conf_stringlist_free(char **stringlist) {
	if (stringlist != NULL) {
		for (char **iterator = stringlist;
			*iterator != NULL; iterator++) {
			free(*iterator);
		}
		free(stringlist);
	}
}

/**
 * Append a string to a NULL-terminated list of strings
 * @param string String to append, allocated previously for this purpose
 * @param stringlist List of strings, can be NULL
 * @param capacity capacity of the list of strings
 * @return List of strings with the string appended
 */
static char **
daemon_conf_stringlist_append(char *string, char **stringlist, size_t *capacity) {
	size_t endindex;

	if (stringlist == NULL) {
		if ((stringlist = malloc(DAEMON_CONF_STRINGLIST_DEFAULT_CAPACITY * sizeof (*stringlist))) == NULL) {
			return NULL;
		}

		*stringlist = NULL;
		*capacity = DAEMON_CONF_STRINGLIST_DEFAULT_CAPACITY;
		endindex = 0;
	} else {
		char **iterator = stringlist;
		while (*iterator != NULL) {
			iterator++;
		}
		endindex = iterator - stringlist;
	}

	if (endindex == *capacity - 1) {
		*capacity *= 2;
		if ((stringlist = realloc(stringlist, *capacity * sizeof (*stringlist))) == NULL) {
			daemon_conf_stringlist_free(stringlist);
			return NULL;
		}
	}

	stringlist[endindex] = string;
	stringlist[endindex + 1] = NULL;

	return stringlist;
}

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
				log_error("daemon_conf: Unable to infer uid from '%s'", user);
				return -1;
			} else {
				*uidp = (uid_t) luid;
			}
		} else {
			log_error("daemon_conf: Unable to infer uid from '%s', getpwnam: %m", user);
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
				log_error("daemon_conf: Unable to infer gid from '%s'", group);
				return -1;
			} else {
				*gidp = (gid_t) lgid;
			}
		} else {
			log_error("daemon_conf: Unable to infer gid from '%s', getgrnam: %m", group);
			return -1;
		}
	} else {
		*gidp = grp->gr_gid;
	}

	return 0;
}

/**
 * Resolves a signal name to a signal number
 * @param signalname nul terminated string of the signal
 * @param signalp Return value of the signal number if the call is successfull
 * @return 0 on success -1 else
 */
static int
daemon_conf_parse_general_signal(const char *signame, int *signump) {

	if (isdigit(*signame)) {
		char *end;
		unsigned long lsignum = strtoul(signame, &end, 10);

		if (*end != '\0' && lsignum <= ((unsigned int)-1 >> 1)) {
			*signump = (int)lsignum;
		} else {
			log_error("daemon_conf: Unable to infer decimal signal from '%s'", signame);
			return -1;
		}
	} else {
		const struct signalpair *current = signals,
			* const signalsend = signals + sizeof (signals) / sizeof (*signals);

		if (strncasecmp("SIG", signame, 3) == 0) {
			signame += 3;
		}

		while (current != signalsend && strcasecmp(current->signame, signame) != 0) {
			current++;
		}

		if (current != signalsend) {
			*signump = current->signum;
		} else {
			log_error("daemon_conf: Unable to infer signal number from name '%s'", signame);
			return -1;
		}
	}

	return 0;
}

/**
 * Determines next argument
 * @param currentp Pointer to the beginning of the previous argument
 * @param currentendp Pointer to the end of the previous argument
 * @return first character of argument or '\0' if end reached
 */
static inline char
daemon_conf_parse_general_arguments_next(const char ** restrict currentp,
	const char ** restrict currentendp) {
	const char *current = *currentp, *currentend = *currentendp;

	for (current = currentend; isspace(*current); current++);
	for (currentend = current; *currentend != '\0' && !isspace(*currentend); currentend++);

	*currentp = current;
	*currentendp = currentend;

	return *current;
}

/**
 * @param args Line of arguments to parse
 * @return List of arguments, NULL terminated
 */
static int
daemon_conf_parse_general_arguments(const char *args, char ***argumentsp) {
	const char *current, *currentend = args;
	char *argument = NULL;
	size_t capacity = 0;

	daemon_conf_stringlist_free(*argumentsp);
	*argumentsp = NULL;

	while (daemon_conf_parse_general_arguments_next(&current, &currentend) != '\0'
		&& (argument = strndup(current, currentend - current)) != NULL
		&& (*argumentsp = daemon_conf_stringlist_append(argument, *argumentsp, &capacity)) != NULL);

	if (argument == NULL) {
		daemon_conf_stringlist_free(*argumentsp);
		*argumentsp = NULL;

		log_error("daemon_conf: Unable to parse argument list '%s'", args);

		return -1;
	}

	return 0;
}

/**
 * Parses a priority, must be between -20 and 20
 * @param prio Priority to parse
 * @param priorityp Pointer to the priority to set
 * @return 0 on success, -1 else
 */
static int
daemon_conf_parse_general_priority(const char *prio, int *priorityp) {
	char *prioend;
	long lpriority = strtol(prio, &prioend, 10);

	if (*prio == '\0' || *prioend != '\0' || lpriority < -20 || lpriority > 20) {
		log_error("daemon_conf: Unable to parse priority '%s'", prio);
		return -1;
	}

	*priorityp = (int)lpriority;

	return 0;
}

/**
 * Parses an element of the general section
 * @param conf Configuration being parsed
 * @param key Key of the parameters value
 * @param value Value associated to key, can be NULL
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

		} else if (strcmp(key, "sigfinish") == 0) {
			if (daemon_conf_parse_general_signal(value, &conf->sigfinish) == -1) {
				return -1;
			}
		} else if (strcmp(key, "sigreload") == 0) {
			if (daemon_conf_parse_general_signal(value, &conf->sigreload) == -1) {
				return -1;
			}
		} else if (strcmp(key, "arguments") == 0) {
			if (daemon_conf_parse_general_arguments(value, &conf->arguments) == -1) {
				return -1;
			}
		} else if (strcmp(key, "priority") == 0) {
			if (daemon_conf_parse_general_priority(value, &conf->priority) == -1) {
				return -1;
			}
		}
	}

	return 0;
}

static char *
daemon_conf_envdup(const char *name) {
	const size_t length = strlen(name);
	char *string = malloc(length * 2 + 2);

	if (string != NULL) {
		*stpncpy(string, name, length) = '=';
		strncpy(string + length + 1, name, length);
	}

	return string;
}

/**
 * Adds an element to the daemon environment
 * @param conf Configuration being parsed
 * @param string String of the form <key>=<value>
 * @param envcapp In-Out value correspondig to the capacity of conf->environment
 * @return 0 on success or unknown key, -1 else
 */
static int
daemon_conf_parse_environment(struct daemon_conf *conf, const char *str, size_t *envcapp) {
	char *string = strchr(str, '=') == NULL ? daemon_conf_envdup(str) : strdup(str);

	if (string != NULL) {
		conf->environment = daemon_conf_stringlist_append(string, conf->environment, envcapp);
	}

	return conf->environment != NULL ? 0 : -1;
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
			conf->start.load = 1;
		} else if (strcmp(key, "reload") == 0) {
			conf->start.reload = 1;
		} else if (strcmp(key, "any exit") == 0) {
			conf->start.exitsuccess = 1;
			conf->start.exitfailure = 1;
			conf->start.killed = 1;
			conf->start.dumped = 1;
		} else if (strcmp(key, "exit") == 0) {
			conf->start.exitsuccess = 1;
			conf->start.exitfailure = 1;
		} else if (strcmp(key, "exit success") == 0) {
			conf->start.exitsuccess = 1;
		} else if (strcmp(key, "exit failure") == 0) {
			conf->start.exitfailure = 1;
		} else if (strcmp(key, "killed") == 0) {
			conf->start.killed = 1;
		} else if (strcmp(key, "dumped") == 0) {
			conf->start.dumped = 1;
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

	conf->sigfinish = SIGTERM;
	conf->sigreload = SIGHUP;

	conf->uid = getuid();
	conf->gid = getgid();

	conf->umask = CONFIG_DEFAULT_UMASK;
	conf->priority = 0;

	conf->start.load = 0;
	conf->start.reload = 0;
	conf->start.exitsuccess = 0;
	conf->start.exitfailure = 0;
	conf->start.killed = 0;
	conf->start.dumped = 0;
}

void
daemon_conf_deinit(struct daemon_conf *conf) {

	free(conf->path);
	free(conf->wd);
	daemon_conf_stringlist_free(conf->arguments);
	daemon_conf_stringlist_free(conf->environment);
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
		if (length != 0 && line[length - 1] == '\n') {
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

