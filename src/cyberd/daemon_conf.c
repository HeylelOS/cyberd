/* SPDX-License-Identifier: BSD-3-Clause */
#include "daemon_conf.h"

#include <stdlib.h> /* abort, free, realloc, ... */
#include <signal.h> /* SIGABRT, ... */
#include <string.h> /* memcpy, strdup, ... */
#include <strings.h> /* strcasecmp, ... */
#include <syslog.h> /* syslog */
#include <alloca.h> /* alloca */
#include <ctype.h> /* isspace, isdigit */
#include <pwd.h> /* getpwnam */
#include <grp.h> /* getgrnam */
#include <errno.h> /* errno */

#ifndef NSIG
#include <limits.h> /* INT_MAX */
/* For platforms without NSIG, just avoid overflows on parsing. */
#define NSIG INT_MAX
#endif

/** Helper macro to describe a signal. */
#define SIGNAL_DESCRIPTION(desc) { SIG##desc, #desc }

struct daemon_conf_value {
	int (* const parse)(struct daemon_conf *, const char *, const char *);
	const char * const key;
};

struct daemon_conf_section {
	const struct daemon_conf_value * const values;
	const char * const name;
};

/*****************************
 * Daemon configuration list *
 *****************************/

/**
 * Append a value at the end of a string list.
 * @param[in,out] listp List of values.
 * @param string Value to append, copied.
 * @returns Zero on success, non-zero else.
 */
static int
daemon_conf_list_append(char ***listp, const char *string) {
	char **list = *listp, *str = strdup(string);
	unsigned int len;

	if (str == NULL) {
		goto strdup_failure;
	}

	if (list != NULL) {
		for (len = 0; list[len] != NULL; len++);
	} else {
		len = 0;
	}

	list = realloc(list, (len + 2) * sizeof (*list));
	if (list == NULL) {
		goto realloc_failure;
	}

	list[len] = str;
	list[len + 1] = NULL;

	*listp = list;

	return 0;
realloc_failure:
	free(str);
strdup_failure:
	return -1;
}

/**
 * Frees a list of strings and all its elements.
 * @param list List of strings to free.
 */
static void
daemon_conf_list_free(char **list) {

	if (list != NULL) {
		for (unsigned int i = 0; list[i] != NULL; i++) {
			free(list[i]);
		}
		free(list);
	}
}

/*************************
 * Parse general section *
 *************************/

/**
 * Replaces a previous NULL or strdup'ed path by a new
 * copy of @param value, which must be an absolute path.
 * @param value Absolute path.
 * @param[out] pathp Replaced path, previous value
 * freed and replaced by a copy of @param value on success.
 * @return Zero on succress, non-zero else.
 */
static int
daemon_conf_path(const char *value, char **pathp) {
	char *path;

	if (value == NULL || *value != '/') {
		return -1;
	}

	path = strdup(value);
	if (path == NULL) {
		return -1;
	}

	free(*pathp);
	*pathp = path;

	return 0;
}

/**
 * Resolves a signal name to a signal number.
 * @param signame Signal name description.
 * @param[out] signalp Returned value of the signal number on success.
 * @return Zero on success, non-zero else.
 */
static int
daemon_conf_signal(const char *signame, int *signop) {
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
	unsigned long lsigno;
	char *end;
	int i;

	if (signame == NULL) {
		return -1;
	}

	if (isdigit(*signame)) {
		lsigno = strtoul(signame, &end, 10);
		if (*end != '\0' || lsigno > NSIG) {
			return -1;
		}
		*signop = (int)lsigno;
		return 0;
	}

	if (strncasecmp("SIG", signame, 3) == 0) {
		signame += 3;
	}

#ifdef CONFIG_DAEMON_CONF_HAS_RTSIG
	if (strncasecmp("RT", signame, 2) == 0) {
		lsigno = strtoul(signame + 2, &end, 10);
		if (*end != '\0' || lsigno > (SIGRTMAX - SIGRTMIN)) {
			return -1;
		}
		*signop = (int)lsigno + SIGRTMIN;
		return 0;
	}
#endif

	i = 0;
	while (i < sizeof (signals) / sizeof (*signals)
		&& strcasecmp(signals[i].name, signame) != 0) {
		i++;
	}

	if (i == sizeof (signals) / sizeof (*signals)) {
		return -1;
	}

	*signop = signals[i].signo;

	return 0;
}

static int
daemon_conf_parse_general_arguments(struct daemon_conf *conf, const char *key, const char *value) {
	enum {
		EXPAND_SPACES,
		EXPAND_LITERAL,
		EXPAND_QUOTE_SINGLE,
		EXPAND_QUOTE_DOUBLE,
		EXPAND_QUOTE_DOUBLE_ESCAPE,
		EXPAND_END,
		EXPAND_ERROR_UNCLOSED_QUOTE,
	} state = EXPAND_SPACES;
	char **arguments, *it, *dst, *src, *arg;

	if (value == NULL) {
		return -1;
	}

	{ /* Make a local copy of value for in-place expansion. */
		const size_t size = strlen(value) + 1;
		it = memcpy(alloca(size), value, size);
	}

	arguments = NULL;
	while (state < EXPAND_END) {
		switch (state) {
		case EXPAND_SPACES:
			switch (*it) {
			case '\0': state = EXPAND_END; break;
			case ' ':  break;
			case '"':  state = EXPAND_QUOTE_DOUBLE; dst = src = arg = it; src++; break;
			case '\'': state = EXPAND_QUOTE_SINGLE; dst = src = arg = it; src++; break;
			default:   state = EXPAND_LITERAL; dst = src = arg = it; dst++; src++; break;
			}
			break;
		case EXPAND_LITERAL:
			switch (*it) {
			case '\0':
				state = EXPAND_END; *dst = '\0';
				if (daemon_conf_list_append(&arguments, arg) != 0) {
					goto failure;
				}
				break;
			case '"':  state = EXPAND_QUOTE_DOUBLE; src++; break;
			case '\'': state = EXPAND_QUOTE_SINGLE; src++; break;
			case ' ':
				state = EXPAND_SPACES; *dst = '\0';
				if (daemon_conf_list_append(&arguments, arg) != 0) {
					goto failure;
				}
				break;
			default:   *dst++ = *src++; break;
			}
			break;
		case EXPAND_QUOTE_SINGLE:
			switch (*it) {
			case '\0': state = EXPAND_ERROR_UNCLOSED_QUOTE; break;
			case '\'': state = EXPAND_LITERAL; src++; break;
			default:   *dst++ = *src++; break;
			}
			break;
		case EXPAND_QUOTE_DOUBLE:
			switch (*it) {
			case '\0': state = EXPAND_ERROR_UNCLOSED_QUOTE; break;
			case '"':  state = EXPAND_LITERAL; src++; break;
			case '\\': state = EXPAND_QUOTE_DOUBLE_ESCAPE; src++; break;
			default:   *dst++ = *src++; break;
			}
			break;
		case EXPAND_QUOTE_DOUBLE_ESCAPE:
			switch (*it) {
			case '\0': state = EXPAND_ERROR_UNCLOSED_QUOTE; break;
			default:   state = EXPAND_QUOTE_DOUBLE; *dst++ = *src++; break;
			}
			break;
		default:
			abort();
		}
		it++;
	}

	if (state == EXPAND_ERROR_UNCLOSED_QUOTE) {
		goto failure;
	}

	if (arguments == NULL) {
		goto failure;
	}

	daemon_conf_list_free(conf->arguments);
	conf->arguments = arguments;

	return 0;
failure:
	daemon_conf_list_free(arguments);
	return -1;
}

static int
daemon_conf_parse_general_group(struct daemon_conf *conf, const char *key, const char *value) {
	struct group *group;

	if (value == NULL) {
		return -1;
	}

	errno = 0;
	group = getgrnam(value);

	if (group != NULL) {
		conf->gid = group->gr_gid;
		return 0;
	}

	if (errno == 0) {
		/* No entry found, treat as decimal gid. */
		char *end;
		const unsigned long lgid = strtoul(value, &end, 10);

		if (*end != '\0' || lgid > CONFIG_DAEMON_CONF_MAX_GID) {
			return -1;
		}

		conf->gid = (gid_t)lgid;
	}

	return 0;
}

static int
daemon_conf_parse_general_nosid(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->nosid = 1;

	return 0;
}

static int
daemon_conf_parse_general_path(struct daemon_conf *conf, const char *key, const char *value) {
	return daemon_conf_path(value, &conf->path);
}

static int
daemon_conf_parse_general_priority(struct daemon_conf *conf, const char *key, const char *value) {
	long lpriority;
	char *end;

	if (value == NULL) {
		return -1;
	}

	lpriority = strtol(value, &end, 10);
	if (*end != '\0' || lpriority < -20 || lpriority > 19) {
		return -1;
	}

	conf->priority = (int)lpriority;

	return 0;
}

static int
daemon_conf_parse_general_sigfinish(struct daemon_conf *conf, const char *key, const char *value) {
	return daemon_conf_signal(value, &conf->sigfinish);
}

static int
daemon_conf_parse_general_sigreload(struct daemon_conf *conf, const char *key, const char *value) {
	return daemon_conf_signal(value, &conf->sigreload);
}

static int
daemon_conf_parse_general_stdin(struct daemon_conf *conf, const char *key, const char *value) {
	return daemon_conf_path(value, &conf->in);
}

static int
daemon_conf_parse_general_stdout(struct daemon_conf *conf, const char *key, const char *value) {
	return daemon_conf_path(value, &conf->out);
}

static int
daemon_conf_parse_general_stderr(struct daemon_conf *conf, const char *key, const char *value) {
	return daemon_conf_path(value, &conf->err);
}

static int
daemon_conf_parse_general_umask(struct daemon_conf *conf, const char *key, const char *value) {
	unsigned long cmask;
	char *end;

	if (value == NULL) {
		return -1;
	}

	cmask = strtoul(value, &end, 8);
	if (*end != '\0') {
		return -1;
	}

	conf->umask = (mode_t)(cmask & 0x1ff);

	return 0;
}

static int
daemon_conf_parse_general_user(struct daemon_conf *conf, const char *key, const char *value) {
	struct passwd *passwd;

	if (value == NULL) {
		return -1;
	}

	errno = 0;
	passwd = getpwnam(value);

	if (passwd != NULL) {
		conf->uid = passwd->pw_uid;
		return 0;
	}

	if (errno == 0) {
		/* No entry found, treat as decimal uid. */
		char *end;
		const unsigned long luid = strtoul(value, &end, 10);

		if (*end != '\0' || luid > CONFIG_DAEMON_CONF_MAX_UID) {
			return -1;
		}

		conf->uid = (uid_t)luid;
	}

	return 0;
}

static int
daemon_conf_parse_general_workdir(struct daemon_conf *conf, const char *key, const char *value) {
	return daemon_conf_path(value, &conf->workdir);
}

/*****************************
 * Parse environment section *
 *****************************/

/**
 * Parse an environment section value.
 * @param conf Configuration being parsed.
 * @param key Key if associative, whole value if scalar.
 * @param value Value if associative, _NULL_ if scalar.
 * @returns Zero on successful parsing of known value, non-zero else.
 */
static int
daemon_conf_parse_environment(struct daemon_conf *conf, const char *key, const char *value) {
	const size_t keylen = strlen(key);
	size_t valuelen, pairsz;
	char *pair;

	if (value != NULL) {
		valuelen = strlen(value);
	} else {
		valuelen = keylen;
		value = key;
	}

	pairsz = keylen + valuelen + 2;
	pair = alloca(pairsz);

	memcpy(pair, key, keylen);
	pair[keylen] = '=';
	memcpy(pair + keylen + 1, value, valuelen + 1);

	return daemon_conf_list_append(&conf->environment, pair);
}

/***********************
 * Parse start section *
 ***********************/

static int
daemon_conf_parse_start_any_exit(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.exitsuccess = 1;
	conf->start.exitfailure = 1;
	conf->start.killed = 1;
	conf->start.dumped = 1;

	return 0;
}

static int
daemon_conf_parse_start_dumped(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.dumped = 1;

	return 0;
}

static int
daemon_conf_parse_start_exit(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.exitsuccess = 1;
	conf->start.exitfailure = 1;

	return 0;
}

static int
daemon_conf_parse_start_exit_failure(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.exitfailure = 1;

	return 0;
}

static int
daemon_conf_parse_start_exit_success(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.exitsuccess = 1;

	return 0;
}

static int
daemon_conf_parse_start_killed(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.killed = 1;

	return 0;
}

static int
daemon_conf_parse_start_load(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.load = 1;

	return 0;
}

static int
daemon_conf_parse_start_reload(struct daemon_conf *conf, const char *key, const char *value) {

	if (value != NULL) {
		return -1;
	}
	conf->start.reload = 1;

	return 0;
}

/***************
 * Daemon conf *
 ***************/

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
	conf->in = NULL;
	conf->out = NULL;
	conf->err = NULL;

	conf->sigfinish = SIGTERM;
	conf->sigreload = SIGHUP;

	conf->uid = 0;
	conf->gid = 0;
	conf->nosid = 0;

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

/***********************
 * Daemon conf parsing *
 ***********************/

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

	{ /* Trimming and comment exclusion. */
		/* Shorten up to the comment if there is one. */
		const char * const comment = strchr(line, '#');
		if (comment != NULL) {
			length = comment - line;
		}

		/* Trim the beginning. */
		while (isspace(*line)) {
			length--;
			line++;
		}

		/* Trim the end. */
		while (isspace(line[length - 1])) {
			length--;
		}

		/* If there wasn't anything to end-trim, no problem,
		 * we are just replacing the previous nul delimiter. */
		line[length] = '\0';
	}

	if (*line == '[' && line[length - 1] == ']') { /* Section delimiter. */
		/* Trim begnning of section name. */
		do {
			length--;
			line++;
		} while (isspace(*line));

		/* Trim end of section name. */
		do {
			length--;
		} while (isspace(line[length - 1]));
		line[length] = '\0';

		*keyp = line;

		return 0;
	} else { /* Scalar or associative value. */
		/* Assign to key. */
		*keyp = line;

		/* Assign to value. */
		char *separator = strchr(line, '=');
		if (separator != NULL) {
			char *preseparator = separator - 1;

			/* Trim the end of key. */
			if (preseparator != line) {
				while (isspace(*preseparator)) {
					preseparator--;
				}
			}
			preseparator[1] = '\0';

			/* Trim the beginning of value. */
			do {
				separator++;
			} while (isspace(*separator));
		}
		*valuep = separator;

		return 1;
	}
}

static const struct daemon_conf_value general_values[] = {
	{ daemon_conf_parse_general_arguments, "arguments" },
	{ daemon_conf_parse_general_group,     "group" },
	{ daemon_conf_parse_general_nosid,     "nosid" },
	{ daemon_conf_parse_general_path,      "path" },
	{ daemon_conf_parse_general_priority,  "priority" },
	{ daemon_conf_parse_general_sigfinish, "sigfinish" },
	{ daemon_conf_parse_general_sigreload, "sigreload" },
	{ daemon_conf_parse_general_stdin,     "stdin" },
	{ daemon_conf_parse_general_stdout,    "stdout" },
	{ daemon_conf_parse_general_stderr,    "stderr" },
	{ daemon_conf_parse_general_umask,     "umask" },
	{ daemon_conf_parse_general_user,      "user" },
	{ daemon_conf_parse_general_workdir,   "workdir" },
	{ },
};

static const struct daemon_conf_value environment_values[] = {
	{ daemon_conf_parse_environment },
};

static const struct daemon_conf_value start_values[] = {
	{ daemon_conf_parse_start_any_exit,     "any exit" },
	{ daemon_conf_parse_start_dumped,       "dumped" },
	{ daemon_conf_parse_start_exit,         "exit" },
	{ daemon_conf_parse_start_exit_failure, "exit failure" },
	{ daemon_conf_parse_start_exit_success, "exit success" },
	{ daemon_conf_parse_start_killed,       "killed" },
	{ daemon_conf_parse_start_load,         "load" },
	{ daemon_conf_parse_start_reload,       "reload" },
	{ },
};

/**
 * Sections and values descriptions, each section group
 * defines a list of matching parsers for each key,
 * the last entry of each list is a 'match-all' entry with a NULL key.
 * Function parsers are optional, and can receive both scalars and associatives values,
 * depending on whether the given value parameter is NULL.
 */
static const struct daemon_conf_section sections[] = {
	{ general_values,     "general" }, /* First one is defaut, begin in general section. */
	{ environment_values, "environment" },
	{ start_values,       "start" },
};

/**
 * Tries parsing a daemon_conf from a file.
 * @param conf Configuration to parse.
 * @param filep File to read and parse from.
 * @return Zero on success, non-zero on failure.
 */
int
daemon_conf_parse(struct daemon_conf *conf, FILE *filep) {
	const struct daemon_conf_section *section = sections;
	char *line = NULL;
	size_t n = 0;

	ssize_t length;
	while (length = getline(&line, &n, filep), length >= 0) {
		const char *key, *value;
		int i;

		if (daemon_conf_parse_line(line, length, &key, &value) == 0) {
			/* Section specifier. */

			i = 0;
			while (i < sizeof (sections) / sizeof (*sections) && strcmp(sections[i].name, key) != 0) {
				i++;
			}

			if (i < sizeof (sections) / sizeof (*sections)) {
				section = sections + i;
			} else {
				section = NULL;
			}
			continue;
		}

		/* Skip if section is not valid,
		 * or if the key is an empty one. */
		if (section == NULL || *key == '\0') {
			continue;
		}

		i = 0;
		while (section->values[i].key != NULL && strcmp(section->values[i].key, key) != 0) {
			i++;
		}

		if (section->values[i].parse != NULL
			&& section->values[i].parse(conf, key, value) != 0) {
			syslog(LOG_ERR, "daemon_conf: Error while parsing key '%s' of section '%s' for value '%s'", key, section->name, value);
		}
	}

	free(line);

	if (conf->path == NULL) {
		syslog(LOG_ERR, "daemon_conf: Missing binary executable path");
		return -1;
	}

	return 0;
}
