#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <err.h>

#include "config.h"

struct cyberctl_args {
	const char *socket;
	const char *newctl;
	const char *when;
	size_t whenlen;
};

static void
cyberctl_usage(const char *cyberctlname) {

	fprintf(stderr, "usage: %s [-s socket] [-t when] <start|stop|reload|end> daemon...\n"
		"       %s [-s socket] [-t when] <poweroff|halt|reboot|suspend>\n"
		"       %s [-s socket] -c <name> [start|stop|reload|end|poweroff|halt|reboot|suspend|cctl]...\n",
		cyberctlname, cyberctlname, cyberctlname);
	exit(EXIT_FAILURE);
}

static int
cyberctl_open(const char *name) {
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);

	if(fd != -1) {
		struct sockaddr_un addr = { .sun_family = AF_LOCAL };
		if(snprintf(addr.sun_path, sizeof(addr.sun_path),
			CONFIG_CONTROLLERS_DIRECTORY"/%s", name) >= sizeof(addr.sun_path)) {
			errx(EXIT_FAILURE, "Socket pathname too long");
		}

		if(connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
			err(EXIT_FAILURE, "connect");
		}
	} else {
		err(EXIT_FAILURE, "socket");
	}

	return fd;
}

static void
cyberctl_close(int fd) {

	shutdown(fd, SHUT_RDWR);
	close(fd);
}

static char *
cyberctl_command_daemon(const char *cyberctlcommand) {

	if(strcmp("start", cyberctlcommand) == 0) {
		return "dstt";
	} else if(strcmp("stop", cyberctlcommand) == 0) {
		return "dstp";
	} else if(strcmp("reload", cyberctlcommand) == 0) {
		return "drld";
	} else if(strcmp("end", cyberctlcommand) == 0) {
		return "dend";
	} else {
		return NULL;
	}
}

static char *
cyberctl_command_system(const char *cyberctlcommand) {

	if(strcmp("poweroff", cyberctlcommand) == 0) {
		return "spwf";
	} else if(strcmp("halt", cyberctlcommand) == 0) {
		return "shlt";
	} else if(strcmp("reboot", cyberctlcommand) == 0) {
		return "srbt";
	} else if(strcmp("suspend", cyberctlcommand) == 0) {
		return "sssp";
	} else {
		return NULL;
	}
}

static void
cyberctl_daemon(int fd,
	const char *command,
	const char *when,
	size_t whenlen,
	const char *daemonname) {
	size_t buffersize = 7 + whenlen + strlen(daemonname);
	char buffer[buffersize];

	snprintf(buffer, buffersize, "%.4s %s %s", command, when, daemonname);
	write(fd, buffer, buffersize);
}

static void
cyberctl_system(int fd,
	const char *command,
	const char *when,
	size_t whenlen) {
	size_t buffersize = 6 + whenlen;
	char buffer[buffersize];

	snprintf(buffer, buffersize, "%.4s %s", command, when);
	write(fd, buffer, buffersize);
}

static void
cyberctl_cctl(int fd,
	char **restricted,
	char **end,
	const char *name) {
	size_t buffersize = 6 + (end - restricted) * 5 + strlen(name);
	char buffer[buffersize];
	char *current = stpncpy(buffer, "cctl ", 5);

	while(restricted != end) {
		char *command = cyberctl_command_daemon(*restricted);

		if(command == NULL) {
			command = cyberctl_command_system(*restricted);
		}

		if(command != NULL || strcmp("cctl", *restricted) == 0) {
			current = stpncpy(current, command, 4);
			*current++ = ' ';
		} else {
			warnx("Unknown restriction '%s'", *restricted);
		}

		restricted++;
	}

	current[-1] = '\t';
	strncpy(current, name, buffer + buffersize - current);

	write(fd, buffer, buffersize);
}

static bool
cyberctl_is_integral(const char *str, const char **strend) {
	const char *current = str;

	while(*current != '\0'
		&& isdigit(*current)) {
		current++;
	}
	*strend = current;

	return current != str && *current == '\0';
}

static struct cyberctl_args
cyberctl_parse_args(int argc, char **argv) {
	struct cyberctl_args args = {
		.socket = "initctl",
		.newctl = getenv("CYBERCTL_SOCKET"),
		.when = "0"
	};
	int c;

	while((c = getopt(argc, argv, ":s:t:c:")) != -1) {
		switch(c) {
		case 's':
			args.socket = optarg;
			break;
		case 't':
			args.when = optarg;
			break;
		case 'c':
			if(*optarg != '\0' && *optarg != '.'
				&& strchr(optarg, '/') == NULL) {
				args.newctl = optarg;
			} else {
				errx(EXIT_FAILURE, "Invalid socket name '%s'", optarg);
			}
			break;
		case '?':
			warnx("Invalid option -%c", optopt);
			cyberctl_usage(*argv);
		case ':':
			warnx("Missing option argument after -%c", optopt);
			cyberctl_usage(*argv);
		}
	}

	const char *whenend;
	if(!cyberctl_is_integral(args.when, &whenend)) {
		warnx("-t option expected to be an integer, found %s", args.when);
		cyberctl_usage(*argv);
	}

	args.whenlen = whenend - args.when;

	if(argc - optind == 0) {
		cyberctl_usage(*argv);
	}

	return args;
}

int
main(int argc,
	char **argv) {
	const struct cyberctl_args args = cyberctl_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;
	int fd = cyberctl_open(args.socket);
	const char *command;

	if(args.newctl != NULL) {
		cyberctl_cctl(fd, argpos, argend, args.newctl);
	} if((command = cyberctl_command_daemon(*argpos)) != NULL) {
		argpos++;

		while(argpos < argend) {
			cyberctl_daemon(fd, command, args.when, args.whenlen, *argpos);
			argpos++;
		}
	} else if((command = cyberctl_command_system(*argpos)) != NULL) {
		if(argend - argpos == 1) {
			cyberctl_system(fd, command, args.when, args.whenlen);
		} else {
			cyberctl_usage(*argv);
		}
	} else {
		warnx("Unknown command '%s'", *argpos);
		cyberctl_usage(*argv);
	}

	cyberctl_close(fd);

	return EXIT_SUCCESS;
}

