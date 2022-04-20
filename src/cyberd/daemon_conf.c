/* SPDX-License-Identifier: BSD-3-Clause */
#include "daemon_conf.h"

#include "log.h"

#include "config.h"

#include <stdlib.h> /* realloc, free, strtol, ... */
#include <signal.h> /* SIGABRT, ... */
#include <string.h> /* strlen, strdup */
#include <unistd.h> /* getuid, getgid */
#include <limits.h> /* INT_MAX */
#include <ctype.h> /* isspace, isdigit */
#include <pwd.h> /* getpwnam */
#include <grp.h> /* getgrnam */
#include <assert.h> /* static_assert */
#include <errno.h> /* errno */

/** Helper macro to describe a signal */
#define SIGNAL_DESCRIPTION(desc) { SIG##desc, #desc }

/** Available sections */
enum daemon_conf_section {
	SECTION_GENERAL,
	SECTION_ENVIRONMENT,
	SECTION_START,
	SECTION_UNKNOWN,
};

/*****************************
 * Daemon configuration list *
 *****************************/

/**
 * Append a value at the end of a string list.
 * @param[in,out] listp List of values.
 * @param string Value to append.
 * @returns Zero on success, non-zero else.
 */
static int
daemon_conf_list_append(char ***listp, char *string) {
	size_t count, newcapacity;
	char **list = *listp;

	if (list != NULL) {
		for (count = 0; list[count] != NULL; count++);
		newcapacity = count + 2;
	} else {
		newcapacity = CONFIG_DAEMON_CONF_LIST_DEFAULT_CAPACITY;
		count = 0;
	}

	list = realloc(list, newcapacity * sizeof (*list));
	if (list == NULL) {
		return -1;
	}

	list[count] = string;
	list[count + 1] = NULL;

	*listp = list;

	return 0;
}

/**
 * Frees a list of strings and all its elements.
 * @param list List of strings to free.
 */
static void
daemon_conf_list_free(char **list) {
	if (list != NULL) {
		for (char **iterator = list; *iterator != NULL; iterator++) {
			free(*iterator);
		}
		free(list);
	}
}

/****************
 * File parsing *
 ****************/

/**
 * Trim and parse a configuration line.
 * @param line Line to trim and parse.
 * @param length Length of @p line.
 * @param[out] keyp Parsed section name or key if associative value, whole value if scalar value.
 * @param[out] valuep Parsed value if associative value, _NULL_ else.
 * @returns Zero on section start, non-zero if value.
 */
static int
daemon_conf_parse_line(char *line, size_t length, const char **keyp, const char **valuep) {

	{ /* Trimming and comment exclusion */
		/* Shorten up to the comment if there is one */
		const char * const comment = strchr(line, '#');
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
		 * we are just replacing the previous nul delimiter */
		line[length] = '\0';
	}

	if (*line == '[' && line[length - 1] == ']') { /* Section delimiter */
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

		return 0;
	} else { /* Scalar or associative value */
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

		return -1;
	}
}

/**
 * Resolves a user name to a user uid.
 * @param user User name or integral id of the desired uid.
 * @param uidp Return value of the uid if the call is successful.
 * @return Zero on success, non-zero else.
 */
static int
daemon_conf_parse_general_uid(const char *user, uid_t *uidp) {
	struct passwd *passwd;

	errno = 0;
	passwd = getpwnam(user);

	if (passwd == NULL) {
		if (errno == 0) {
			/* No entry found, treat as numeric gid */
			char *end;
			unsigned long luid = strtoul(user, &end, 10);

			if (*user == '\0' || *end != '\0') {
				log_error("daemon_conf: Unable to infer uid from '%s'", user);
				return -1;
			} else {
				*uidp = (uid_t)luid;
			}
		} else {
			log_error("daemon_conf: Unable to infer uid from '%s', getpwnam: %m", user);
			return -1;
		}
	} else {
		*uidp = passwd->pw_uid;
	}

	return 0;
}

/**
 * Resolves a group name to a group gid.
 * @param user Group name or integral id of the desired gid.
 * @param[out] gidp Returned value of the gid on success.
 * @return Zero on success, non-zero else.
 */
static int
daemon_conf_parse_general_gid(const char *group, gid_t *gidp) {
	struct group *grp;

	errno = 0;
	grp = getgrnam(group);

	if (grp == NULL) {
		if (errno == 0) {
			/* No entry found, treat as numeric gid */
			char *end;
			unsigned long lgid = strtoul(group, &end, 10);

			if (*group == '\0' || *end != '\0') {
				log_error("daemon_conf: Unable to infer gid from '%s'", group);
				return -1;
			} else {
				*gidp = (gid_t)lgid;
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
 * Resolves a signal name to a signal number.
 * @param signame Signal name description.
 * @param[out] signalp Returned value of the signal number on success.
 * @return Zero on success, non-zero else.
 */
static int
daemon_conf_parse_general_signal(const char *signame, int *signop) {

	if (isdigit(*signame)) {
		char *end;
		const unsigned long lsignum = strtoul(signame, &end, 10);

		if (*end != '\0' || lsignum > INT_MAX) {
			log_error("daemon_conf: Unable to infer decimal signal from '%s'", signame);
			return -1;
		}

		*signop = (int)lsignum;
	} else {
		static const struct {
			const int signo;
			const char name[8];
		} signals[] = {
			SIGNAL_DESCRIPTION(ABRT),
			SIGNAL_DESCRIPTION(ABRT),
			SIGNAL_DESCRIPTION(ALRM),
			SIGNAL_DESCRIPTION(BUS),
			SIGNAL_DESCRIPTION(CHLD),
			SIGNAL_DESCRIPTION(CONT),
			SIGNAL_DESCRIPTION(FPE),
			SIGNAL_DESCRIPTION(HUP),
			SIGNAL_DESCRIPTION(ILL),
			SIGNAL_DESCRIPTION(INT),
			SIGNAL_DESCRIPTION(KILL),
			SIGNAL_DESCRIPTION(PIPE),
			SIGNAL_DESCRIPTION(QUIT),
			SIGNAL_DESCRIPTION(SEGV),
			SIGNAL_DESCRIPTION(STOP),
			SIGNAL_DESCRIPTION(TERM),
			SIGNAL_DESCRIPTION(TSTP),
			SIGNAL_DESCRIPTION(TTIN),
			SIGNAL_DESCRIPTION(TTOU),
			SIGNAL_DESCRIPTION(USR1),
			SIGNAL_DESCRIPTION(USR2),
			SIGNAL_DESCRIPTION(POLL),
			SIGNAL_DESCRIPTION(PROF),
			SIGNAL_DESCRIPTION(SYS),
			SIGNAL_DESCRIPTION(TRAP),
			SIGNAL_DESCRIPTION(URG),
			SIGNAL_DESCRIPTION(VTALRM),
			SIGNAL_DESCRIPTION(XCPU),
			SIGNAL_DESCRIPTION(XFSZ),
		};
		unsigned int i = 0;

		if (strncasecmp("SIG", signame, 3) == 0) {
			signame += 3;
		}

		while (i < sizeof (signals) / sizeof (*signals) && strcasecmp(signals[i].name, signame) != 0) {
			i++;
		}

		if (i == sizeof (signals) / sizeof (*signals)) {
			log_error("daemon_conf: Unable to infer signal number from name '%s'", signame);
			return -1;
		}

		*signop = signals[i].signo;
	}

	return 0;
}

/**
 * Determine next argument.
 * @param currentp Pointer to the beginning of the previous argument.
 * @param currentendp Pointer to the end of the previous argument.
 * @return first character of argument or '\0' if end reached.
 */
static inline char
daemon_conf_parse_general_arguments_next(const char ** restrict currentp, const char ** restrict currentendp) {
	const char *current = *currentp, *currentend = *currentendp;

	for (current = currentend; isspace(*current); current++);
	for (currentend = current; *currentend != '\0' && !isspace(*currentend); currentend++);

	*currentp = current;
	*currentendp = currentend;

	return *current;
}

/**
 * Parse an argument array.
 * @param args Line of arguments to parse.
 * @returns List of arguments, NULL terminated.
 */
static int
daemon_conf_parse_general_arguments(const char *args, char ***argumentsp) {
	const char *current, *currentend = args;
	char **list = NULL, *argument = NULL;

	while (daemon_conf_parse_general_arguments_next(&current, &currentend) != '\0'
		&& (argument = strndup(current, currentend - current)) != NULL
		&& daemon_conf_list_append(&list, argument) == 0);

	if (argument == NULL) {
		daemon_conf_list_free(list);
		log_error("daemon_conf: Unable to parse argument list '%s'", args);
		return -1;
	}

	daemon_conf_list_free(*argumentsp);
	*argumentsp = list;

	return 0;
}

/**
 * Parse a priority, must be between -20 and 19.
 * @param prio Priority to parse.
 * @param[out] priorityp Returned priority on success.
 * @return Zero on success, non-zero else.
 */
static int
daemon_conf_parse_general_priority(const char *prio, int *priorityp) {
	char *end;
	long lpriority = strtol(prio, &end, 10);

	if (*prio == '\0' || *end != '\0' || lpriority < -20 || lpriority > 19) {
		log_error("daemon_conf: Unable to parse priority '%s'", prio);
		return -1;
	}

	*priorityp = (int)lpriority;

	return 0;
}

/**
 * Parse a general section value.
 * @param conf Configuration being parsed.
 * @param key Key if associative, whole value if scalar.
 * @param value Value if associative, _NULL_ if scalar.
 * @returns Zero on successful parsing of known value, non-zero else.
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
	const char * const *current = keys, * const *keysend = keys + sizeof (keys) / sizeof (*keys);
	int retval = 0;

	while (current != keysend && strcmp(*current, key) != 0) {
		current++;
	}

	if (current != keysend) {
		if (value != NULL) {
			switch (current - keys) {
			case 0: { /* path */
				char * const newpath = strdup(value);

				if (newpath != NULL) {
					free(conf->path);
					conf->path = newpath;
				} else {
					retval = -1;
				}
			}	break;
			case 1: /* workdir */
				if (*value == '/') {
					char * const workdir = strdup(value);

					if (workdir != NULL) {
						free(conf->workdir);
						conf->workdir = workdir;
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
				retval = -1; /* Invalid state, should not happen */
				break;
			}
		} else {
			retval = -1; /* No scalar values in general section */
		}
	} else {
		retval = -1; /* Invalid key */
	}

	return retval;
}

/**
 * Create an environment array value.
 * @param key Key of the value.
 * @param value Value, or _NULL_.
 * @returns A duplicate string, if @p value is _NULL_ "<key>=<key>", else "<key>=<value>".
 */
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

	memcpy(string, key, keylength);
	string[keylength - 1] = '='; /* We're safe, as we discarded empty keys earlier */
	memcpy(string + keylength + 1, value, valuelength + 1);

	return strdup(string);
}

/**
 * Parse an environment section value.
 * @param conf Configuration being parsed.
 * @param key Key if associative, whole value if scalar.
 * @param value Value if associative, _NULL_ if scalar.
 * @returns Zero on successful parsing of known value, non-zero else.
 */
static int
daemon_conf_parse_environment(struct daemon_conf *conf, const char *key, const char *value) {
	char * const string = daemon_conf_envdup(key, value);
	int retval = 0;

	if (string != NULL) {
		if (daemon_conf_list_append(&conf->environment, string) != 0) {
			free(string);
			retval = -1;
		}
	} else {
		retval = -1;
	}

	return retval;
}

/**
 * Parse a start section value.
 * @param conf Configuration being parsed.
 * @param key Key if associative, whole value if scalar.
 * @param value Value if associative, _NULL_ if scalar.
 * @returns Zero on successful parsing of known value, non-zero else.
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
	const char * const *current = keys, * const *keysend = keys + sizeof (keys) / sizeof (*keys);
	int retval = 0;

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
				retval = -1; /* Invalid state, should not happen */
				break;
			}
		} else {
			retval = -1; /* No associative values in start section */
		}
	} else {
		retval = -1; /* Invalid key */
	}

	return retval;
}

/**
 * Initializes a configuration for later parsing with defaults.
 * @param conf Configuration to initialize.
 */
void
daemon_conf_init(struct daemon_conf *conf) {

	conf->path = NULL;
	conf->arguments = NULL;
	conf->environment = NULL;
	conf->workdir = NULL;

	conf->sigfinish = SIGTERM;
	conf->sigreload = SIGHUP;

#ifdef NDEBUG
	conf->uid = 0;
	conf->gid = 0;
#else
	conf->uid = getuid();
	conf->gid = getgid();
#endif

	conf->umask = CONFIG_DAEMON_CONF_DEFAULT_UMASK;
	conf->priority = 0;

	conf->start.load = 0;
	conf->start.reload = 0;
	conf->start.exitsuccess = 0;
	conf->start.exitfailure = 0;
	conf->start.killed = 0;
	conf->start.dumped = 0;
}

/**
 * Deinitializes a configuration, freeing all used data.
 * @param conf Configuration to deinitialize.
 */
void
daemon_conf_deinit(struct daemon_conf *conf) {
	free(conf->path);
	free(conf->workdir);
	daemon_conf_list_free(conf->arguments);
	daemon_conf_list_free(conf->environment);
}

/**
 * Tries parsing a daemon_conf from a file.
 * @param conf Configuration to parse.
 * @param filep File to read and parse from.
 * @return Zero on success, non-zero on failure.
 */
int
daemon_conf_parse(struct daemon_conf *conf, FILE *filep) {
	enum daemon_conf_section section = SECTION_GENERAL;
	size_t linecap = 0;
	char *line = NULL;
	int retval = 0;
	ssize_t length;

	while (length = getline(&line, &linecap, filep), length >= 0) {
		const char *key, *value;

		if (daemon_conf_parse_line(line, length, &key, &value) == 0) {
			static const char * const sections[] = {
				[SECTION_GENERAL]     = "general",
				[SECTION_ENVIRONMENT] = "environment",
				[SECTION_START]       = "start",
			};
			const char * const *current = sections, * const *sectionsend = sections + sizeof (sections) / sizeof (*sections);

			static_assert(sizeof (sections) / sizeof (*sections) == SECTION_UNKNOWN, "Missing section description");

			while (current != sectionsend && strcmp(*current, key) != 0) {
				current++;
			}

			/* SECTION_UNKNOWN being the next after the last, it corresponds to sectionsend */
			section = current - sections;

		} else if (*key != '\0') { /* We don't take empty keys */
			switch (section) {
			case SECTION_GENERAL:
				if (daemon_conf_parse_general(conf, key, value) != 0) {
					retval = -1;
				}
				break;
			case SECTION_ENVIRONMENT:
				if (daemon_conf_parse_environment(conf, key, value) != 0) {
					retval = -1;
				}
				break;
			case SECTION_START:
				if (daemon_conf_parse_start(conf, key, value) != 0) {
					retval = -1;
				}
				break;
			case SECTION_UNKNOWN:
				break;
			}
		}
	}

	free(line);

	if (conf->path == NULL) {
		log_error("daemon_conf: Missing binary executable path");
		retval = -1;
	}

	return retval;
}
