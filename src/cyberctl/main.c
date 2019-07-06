#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <err.h>

#include "config.h"

struct cyberctl_args {
	const char *socket;
	const char *newctl;
	time_t when;
};

static void
cyberctl_usage(const char *cyberctlname) {

	fprintf(stderr, "usage: %s [-s socket] [-n] [-t when] <start|stop|reload|end> daemon...\n"
		"       %s [-s socket] [-n] [-t when] <poweroff|halt|reboot|suspend>\n"
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
			err(EXIT_FAILURE, "connect %s", addr.sun_path);
		}
	} else {
		err(EXIT_FAILURE, "socket %s", name);
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
	time_t when,
	const char *daemonname) {
	size_t buffersize = 27 + strlen(daemonname);
	char buffer[buffersize];
	size_t bufferlen;

	if((bufferlen = snprintf(buffer, buffersize, "%.4s %.20ld %s", command, when, daemonname)) < buffersize) {
		bufferlen++;

		if(write(fd, buffer, bufferlen) != bufferlen) {
			warn("Unable to write");
		}
	}
}

static void
cyberctl_system(int fd,
	const char *command,
	time_t when) {
	size_t buffersize = 26;
	char buffer[buffersize];
	size_t bufferlen;

	if((bufferlen = snprintf(buffer, buffersize, "%.4s %.20ld", command, when)) < buffersize) {
		bufferlen++;

		if(write(fd, buffer, bufferlen) != bufferlen) {
			warn("Unable to write");
		}
	}
}

static void
cyberctl_cctl(int fd,
	char **restricted,
	char **end,
	const char *name) {
	size_t buffersize = 6 + (end - restricted) * 5 + strlen(name);
	char buffer[buffersize];
	char *current = stpncpy(buffer, "cctl ", 5);
	size_t bufferlen;

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
	current = stpncpy(current, name, buffer + buffersize - current);
	bufferlen = current - buffer;
	if(write(fd, buffer, bufferlen) != bufferlen) {
		warn("Unable to write");
	}
}

static struct cyberctl_args
cyberctl_parse_args(int argc, char **argv) {
	struct cyberctl_args args = {
		.socket = "initctl",
		.newctl = getenv("CYBERCTL_SOCKET"),
		.when = 0
	};
	bool sinceepoch = true;
	int c;

	while((c = getopt(argc, argv, ":s:t:c:n")) != -1) {
		switch(c) {
		case 's':
			args.socket = optarg;
			break;
		case 't': {
			char *end;
			unsigned long value = strtoul(optarg, &end, 10);

			if(*end == '\0' && *optarg != '\0') {
				args.when = (time_t) value;
				if(args.when < 0) {
					args.when = 0;
				}
			} else {
				warnx("Expected integral value for -%c, found '%s' (stopped at '%c')",
					optopt, optarg, *end);
				cyberctl_usage(*argv);
			}
		} break;
		case 'c':
			if(*optarg != '\0' && *optarg != '.'
				&& strchr(optarg, '/') == NULL) {
				args.newctl = optarg;
			} else {
				errx(EXIT_FAILURE, "Invalid socket name '%s'", optarg);
			}
			break;
		case 'n':
			sinceepoch = false;
			break;
		case '?':
			warnx("Invalid option -%c", optopt);
			cyberctl_usage(*argv);
		case ':':
			warnx("Missing option argument after -%c", optopt);
			cyberctl_usage(*argv);
		}
	}

	if(!sinceepoch) {
		args.when += time(NULL);
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
	} else if(argc - optind > 0) {
		if((command = cyberctl_command_daemon(*argpos)) != NULL) {
			argpos++;

			if(argpos != argend) {
				do {
					cyberctl_daemon(fd, command, args.when, *argpos);
					argpos++;
				} while(argpos != argend);
			} else {
				warnx("No daemon(s) specified for command '%s'", argpos[-1]);
				cyberctl_usage(*argv);
			}
		} else if((command = cyberctl_command_system(*argpos)) != NULL) {
			if(argend - argpos == 1) {
				cyberctl_system(fd, command, args.when);
			} else {
				warnx("Unexpected '%s' after '%s'", argpos[1], *argpos);
				cyberctl_usage(*argv);
			}
		} else {
			warnx("Unknown command '%s'", *argpos);
			cyberctl_usage(*argv);
		}
	} else {
		warnx("Missing argument");
		cyberctl_usage(*argv);
	}

	cyberctl_close(fd);

	return EXIT_SUCCESS;
}

