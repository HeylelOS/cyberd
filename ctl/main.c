#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "../config.h"

static const char *cyberctlname;

static void
cyberctl_usage(void) {

	fprintf(stderr, "usage: %s [-t when] <start|stop|reload|end> daemon...\n"
		"       %s [-t when] <poweroff|halt|reboot|suspend>\n"
		"       %s cctl <name> [start|stop|reload|end|poweroff|halt|reboot|suspend]...\n",
		cyberctlname, cyberctlname, cyberctlname);
	exit(EXIT_FAILURE);
}

static void
cyberctl_error(const char *name) {

	fprintf(stderr, "%s: error %s: %s\n",
		cyberctlname, name, strerror(errno));
	exit(EXIT_FAILURE);
}

static int
cyberctl_open(const char *path) {
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);

	if(fd != -1) {
		struct sockaddr_un addr = { .sun_family = AF_LOCAL };
		strncpy(addr.sun_path, path, sizeof(addr.sun_path));

		if(connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
			cyberctl_error("connect");
		}
	} else {
		cyberctl_error("socket");
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
	const char **restricted,
	const char **end,
	const char *name) {
	size_t buffersize = 6 + (end - restricted) * 5 + strlen(name);
	char buffer[buffersize];
	char *current = stpncpy(buffer, "cctl ", 5);

	while(restricted != end) {
		char *command = cyberctl_command_daemon(*restricted);

		if(command == NULL) {
			command = cyberctl_command_system(*restricted);
		}

		if(command == NULL && strcmp("cctl", *restricted) != 0) {
			cyberctl_usage();
		}

		current = stpncpy(current, command, 4);
		*current++ = ' ';

		restricted += 1;
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
		current += 1;
	}
	*strend = current;

	return current != str && *current == '\0';
}

int
main(int argc,
	const char **argv) {
	const char **argpos = argv + 1;
	const char **argend = argv + argc;
	cyberctlname = *argv;

	if(argpos == argend) {
		cyberctl_usage();
	}

	const char *when = "0";
	const char *whenend;
	if(strcmp("-t", *argpos) == 0
		&& argpos + 2 < argend
		&& cyberctl_is_integral(argpos[1], &whenend)) {
		when = argpos[1];
		argpos += 2;
	} else {
		whenend = when + 1;
	}

	int fd = cyberctl_open(CONFIG_INITCTL_PATH);
	size_t whenlen = whenend - when;
	const char *command;

	if((command = cyberctl_command_daemon(*argpos)) != NULL) {
		argpos += 1;

		while(argpos < argend) {
			cyberctl_daemon(fd, command, when, whenlen, *argpos);
			argpos += 1;
		}
	} else if(argend - argpos == 1
		&&(command = cyberctl_command_system(*argpos)) != NULL) {

		cyberctl_system(fd, command, when, whenlen);
	} else if(strcmp("cctl", *argpos) == 0) {
		argpos += 1;

		cyberctl_cctl(fd, argpos + 1, argend, *argpos);
	} else {
		cyberctl_usage();
	}

	cyberctl_close(fd);

	return EXIT_SUCCESS;
}

