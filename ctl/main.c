#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <alloca.h>
#include <errno.h>

#include "../config.h"

static const char *cyberctlname;

static void
cyberctl_usage(void) {
	fprintf(stderr, "usage: %s ...\n",
		cyberctlname);
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

	if (fd != -1) {
		struct sockaddr_un addr = { .sun_family = AF_LOCAL };
		strncpy(addr.sun_path, path, sizeof (addr.sun_path));

		if (connect(fd, (const struct sockaddr *)&addr, sizeof (addr)) != 0) {
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

static bool
cyberctl_is_integral(const char *str, const char **strend) {
	const char *current = str;

	while (*current != '\0' && isdigit(*current)) {
		current += 1;
	}

	*strend = current;

	return current != str && *current == '\0';
}

static void
cyberctl_planified(int fd,
	const char *command,
	const char *when,
	const char *daemonname) {
	const char *whenend;

	if (cyberctl_is_integral(when, &whenend)) {
		size_t buffersize = 3
			+ strlen(command)
			+ whenend - when
			+ strlen(daemonname);
		char *buffer = alloca(buffersize);

		snprintf(buffer, buffersize, "%s %s %s", command, when, daemonname);
		write(fd, buffer, buffersize);
	} else {
		cyberctl_usage();
	}
}

int
main(int argc,
	char **argv) {
	cyberctlname = *argv;
	int fd = cyberctl_open(CONFIG_INITCTL_PATH);
	char c;

	cyberctl_planified(fd, "dstt", "0", "test.valid.load");
	cyberctl_planified(fd, "drld", "0", "test.sleep.10");
	cyberctl_planified(fd, "dend", "0", "test.sleep.5");
	cyberctl_planified(fd, "dstp", "0", "test.sleep.10");
	read(0, &c, 1);

	cyberctl_close(fd);

	return EXIT_SUCCESS;
}

