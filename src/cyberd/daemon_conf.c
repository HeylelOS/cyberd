#include "daemon_conf.h"
#include "log.h"

#include "config.h"

#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <errno.h>

#define DAEMON_CONF_ARRAY_SIZE(array) (sizeof ((array)) / sizeof *(array))

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

static const char * const sections[] = {
	"general",
	"environment",
	"start"
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
 * Trims and parse a configuration file line.
 * @param line Line to parse, length of @p length.
 * @param length Length of @p line.
 * @param keyp Out variable corresponding to the key of the value, the scalar value or the section name.
 * @param valuep Out variable corresponding to the associated value of the key, NULL if @p keyp is a scalar value, unchanged if @p keyp is a section name.
 * @return true if @p keyp holds a section name, false if it holds a key or a scalar value.
 */
static bool
daemon_conf_parse_line(char *line, size_t length, const char **keyp, const char **valuep) {
	/* Shorten up to the comment if there is one */
	const char *comment = strchr(line, '#');
	if (comment != NULL) {
		length = comment - line;
	}

	/* Trim the beginning */
	while (isspace(*line)) {
		length--;
		line++;
	}

	/* Trim the end */
	while (isspace(line[length - 1])) {
		length--;
	}
	/* If there wasn't anything to end-trim, no problem,
	we will just be replacing the previous nul delimiter */
	line[length] = '\0';

	if (*line == '[' && line[length - 1] == ']') {
		/* We are a new section delimiter */
		/* Trim begnning of section name */
		do {
			length--;
			line++;
		} while (isspace(*line));

		/* Trim end of section name */
		do {
			length--;
		} while (isspace(line[length - 1]));
		line[length] = '\0';

		*keyp = line;

		return true;
	}

	/* Assign to key */
	*keyp = line;

	/* Assign to value */
	char *separator = strchr(line, '=');
	if (separator != NULL) {
		char *preseparator = separator - 1;

		/* Trim the end of key */
		if (preseparator != line) {
			while (isspace(*preseparator)) {
				preseparator--;
			}
		}
		preseparator[1] = '\0';

		/* Trim the beginning of value */
		do {
			separator++;
		} while (isspace(*separator));
	}
	*valuep = separator;

	return false;
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
			* const signalsend = signals + DAEMON_CONF_ARRAY_SIZE(signals);

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
 * Parses a priority, must be between -20 and 19
 * @param prio Priority to parse
 * @param priorityp Pointer to the priority to set
 * @return 0 on success, -1 else
 */
static int
daemon_conf_parse_general_priority(const char *prio, int *priorityp) {
	char *prioend;
	long lpriority = strtol(prio, &prioend, 10);

	if (*prio == '\0' || *prioend != '\0' || lpriority < -20 || lpriority > 19) {
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
daemon_conf_parse_general(struct daemon_conf *conf, const char *key, const char *value) {
	static const char * const keys[] = {
		"path",
		"workdir",
		"user",
		"group",
		"umask",
		"sigfinish",
		"sigreload",
		"arguments",
		"priority"
	};

	const char * const *current = keys,
		* const *keysend = keys + DAEMON_CONF_ARRAY_SIZE(keys);

	while (current != keysend && strcmp(*current, key) != 0) {
		current++;
	}

	int retval;
	if (current != keysend) {
		if (value != NULL) {
			switch (current - keys) {
			case 0: { /* path */
				char * const newpath = strdup(value);

				if (newpath != NULL) {
					retval = 0;
					free(conf->path);
					conf->path = newpath;
				} else {
					retval = -1;
				}

			}	break;
			case 1: /* workdir */
				if (*value == '/') {
					char * const newwd = strdup(value);

					if (newwd != NULL) {
						free(conf->wd);
						conf->wd = newwd;
						retval = 0;
						break;
					}
				}
				retval = -1;
				break;
			case 2: /* user */
				retval = daemon_conf_parse_general_uid(value, &conf->uid);
				break;
			case 3: /* group */
				retval = daemon_conf_parse_general_gid(value, &conf->gid);
				break;
			case 4: { /* umask */
				char *valueend;
				const unsigned long cmask = strtoul(value, &valueend, 8);

				if (*value != '\0' && *valueend == '\0') {
					retval = 0;
					conf->umask = cmask & 0x1FF;
				} else {
					retval = -1;
				}

			}	break;
			case 5: /* sigfinish */
				retval = daemon_conf_parse_general_signal(value, &conf->sigfinish);
				break;
			case 6: /* sigreload */
				retval = daemon_conf_parse_general_signal(value, &conf->sigreload);
				break;
			case 7: /* arguments */
				retval = daemon_conf_parse_general_arguments(value, &conf->arguments);
				break;
			case 8: /* priority */
				retval = daemon_conf_parse_general_priority(value, &conf->priority);
				break;
			default:
				break;
			}
		}
	}

	return retval;
}

static char *
daemon_conf_envdup(const char *key, const char *value) {
	const size_t keylength = strlen(key);
	size_t valuelength;

	if (value != NULL) {
		valuelength = strlen(value);
	} else {
		valuelength = keylength;
		value = key;
	}

	char string[keylength + valuelength + 2];

	*stpncpy(string, key, keylength) = '=';
	strncpy(string + keylength + 1, value, valuelength + 1);

	return strndup(string, sizeof (string));
}

/**
 * Adds an element to the daemon environment
 * @param conf Configuration being parsed
 * @param key Key for the environment variable
 * @param value Value associated with @p key for the environment variable, or NULL
 * @param envcapp In-Out value corresponding to the capacity of conf->environment
 * @return 0 on success, -1 else
 */
static int
daemon_conf_parse_environment(struct daemon_conf *conf, const char *key, const char *value, size_t *envcapp) {
	char * const string = daemon_conf_envdup(key, value);
	int retval = -1;

	if (string != NULL) {
		conf->environment = daemon_conf_stringlist_append(string, conf->environment, envcapp);
		if (conf->environment != NULL) {
			retval = 0;
		}
	}

	return retval;
}

/**
 * Parses an element of the start section
 * @param conf Configuration being parsed
 * @key Key of the parameters value
 * @value Value associated to key, can be NULL
 * @return 0 on success or unknown key, -1 else
 */
static int
daemon_conf_parse_start(struct daemon_conf *conf, const char *key, const char *value) {
	static const char * const keys[] = {
		"load",
		"reload",
		"any exit",
		"exit",
		"exit success",
		"exit failure",
		"killed",
		"dumped"
	};

	const char * const *current = keys,
		* const *keysend = keys + DAEMON_CONF_ARRAY_SIZE(keys);

	while (current != keysend && strcmp(*current, key) != 0) {
		current++;
	}

	if (current != keysend) {
		if (value == NULL) {
			switch (current - keys) {
			case 0: /* load */
				conf->start.load = 1;
				break;
			case 1: /* reload */
				conf->start.reload = 1;
				break;
			case 2: /* any exit */
				conf->start.exitsuccess = 1;
				conf->start.exitfailure = 1;
				conf->start.killed = 1;
				conf->start.dumped = 1;
				break;
			case 3: /* exit */
				conf->start.exitsuccess = 1;
				conf->start.exitfailure = 1;
				break;
			case 4: /* exit success */
				conf->start.exitsuccess = 1;
				break;
			case 5: /* exit failure */
				conf->start.exitfailure = 1;
				break;
			case 6: /* killed */
				conf->start.killed = 1;
				break;
			case 7: /* dumped */
				conf->start.dumped = 1;
				break;
			default:
				break;
			}
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

	conf->umask = CONFIG_DAEMONS_DEFAULT_UMASK;
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
daemon_conf_parse(struct daemon_conf *conf, FILE *filep) {
	enum daemon_conf_section section = SECTION_GENERAL;
	size_t linecap = 0, envcap = 0;
	char *line = NULL;
	int errors = 0;
	ssize_t length;

	/* Reading lines */
	while (length = getline(&line, &linecap, filep), length != -1) {
		const char *key, *value;

		if (daemon_conf_parse_line(line, length, &key, &value)) {
			const char * const *current = sections,
				* const *sectionsend = sections + DAEMON_CONF_ARRAY_SIZE(sections);

			while (current != sectionsend && strcmp(*current, key) != 0) {
				current++;
			}

			/* SECTION_UNKNOWN being the next after the last, it corresponds to sectionsend */
			section = current - sections;

		} else if (*key != '\0') { /* We don't take empty keys */
			switch (section) {
			case SECTION_GENERAL:
				if (daemon_conf_parse_general(conf, key, value) != 0) {
					errors++;
				}
				break;
			case SECTION_ENVIRONMENT:
				if (daemon_conf_parse_environment(conf, key, value, &envcap) != 0) {
					errors++;
				}
				break;
			case SECTION_START:
				if (daemon_conf_parse_start(conf, key, value) != 0) {
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

