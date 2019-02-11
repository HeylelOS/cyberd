#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "../config.h"

static const char *cyberctlname;

static void
cyberctl_error(const char *name) {
	fprintf(stderr, "%s: error %s: %s\n",
		cyberctlname, name, strerror(errno));
	exit(EXIT_FAILURE);
}

int
main(int argc,
	char **argv) {
	const struct sockaddr_un addr = {
		.sun_family = AF_LOCAL,
		.sun_path = CONFIG_INITCTL_PATH
	};
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	cyberctlname = *argv;

	if(fd != -1) {
		if(connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) == 0) {
			char buffer[512];
			ssize_t readval;

			while((readval = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
				write(fd, buffer, readval);
			}

			shutdown(fd, SHUT_RDWR);
			close(fd);
		} else {
			cyberctl_error("connect");
		}
	} else {
		cyberctl_error("socket");
	}

	return EXIT_SUCCESS;
}

