/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <err.h>

#include "capabilities.h"

#include "config.h"

#ifndef __has_builtin
#error "Builtin macro __has_builtin is not available"
#endif

#if !__has_builtin(__builtin_ctz)
#error "Builtin __builtin_ctz is not available"
#endif

#if !__has_c_attribute(gnu::packed)
#error "Attribute gnu::packed is not available"
#endif

#define COMMAND(command) __builtin_ctz(CAPABILITY_##command)

enum synopsis {
	SYNOPSIS_HALT,
	SYNOPSIS_REBOOT,
	SYNOPSIS_SUSPEND,
	SYNOPSIS_POWEROFF,
	SYNOPSIS_SHUTDOWN,
	SYNOPSIS_INITCTL,
};

static uint8_t
initctl_command_id(const char *command) {
	const char * const commands[] = {
		[COMMAND(ENDPOINT_CREATE)] = "create-endpoint",
		[COMMAND(DAEMON_START)] = "start",
		[COMMAND(DAEMON_STOP)] = "stop",
		[COMMAND(DAEMON_RELOAD)] = "reload",
		[COMMAND(DAEMON_END)] = "end",
		[COMMAND(SYSTEM_POWEROFF)] = "poweroff",
		[COMMAND(SYSTEM_HALT)] = "halt",
		[COMMAND(SYSTEM_REBOOT)] = "reboot",
		[COMMAND(SYSTEM_SUSPEND)] = "suspend",
	};
	uint8_t id = 0;

	while (id < sizeof (commands) / sizeof (*commands) && strcmp(commands[id], command) != 0) {
		id++;
	}

	return id;
}

static int
initctl_open(const char *endpoint) {
	const int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	struct sockaddr_un addr = { .sun_family = AF_LOCAL };

	if (fd < 0) {
		err(EXIT_FAILURE, "Unable to create socket for endpoint '%s'", endpoint);
	}

	if (snprintf(addr.sun_path, sizeof (addr.sun_path), CONFIG_ENDPOINTS_DIRECTORY"/%s", endpoint) >= sizeof (addr.sun_path)) {
		errx(EXIT_FAILURE, "Endpoint name '%s' is too long", endpoint);
	}

	if (connect(fd, (const struct sockaddr *)&addr, sizeof (addr)) != 0) {
		err(EXIT_FAILURE, "Unable to connect to endpoint '%s'", endpoint);
	}

	return fd;
}

static void noreturn
initctl_endpoint_create(const char *endpoint, uint8_t id, capset_t restricted, const char *name) {
	struct [[gnu::packed]] {
		uint8_t id;
		uint32_t restricted;
		char name[];
	} *message;
	const size_t namelen = strlen(name);
	const size_t messagesize = sizeof (*message) + namelen + 1;
	const int fd = initctl_open(endpoint);

	message = alloca(messagesize);
	message->id = id;
	message->restricted = htonl(restricted);
	memcpy(message->name, name, namelen + 1);

	if (write(fd, message, messagesize) != messagesize) {
		err(EXIT_FAILURE, "Unable to write to endpoint");
	}

	exit(EXIT_SUCCESS);
}

static void noreturn
initctl_daemon(const char *endpoint, uint8_t id, const char *name) {
	struct [[gnu::packed]] {
		uint8_t id;
		char name[];
	} *message;
	const size_t namelen = strlen(name);
	const size_t messagesize = sizeof (*message) + namelen + 1;
	const int fd = initctl_open(endpoint);

	message = alloca(messagesize);
	message->id = id;
	memcpy(message->name, name, namelen + 1);

	if (write(fd, message, messagesize) != messagesize) {
		err(EXIT_FAILURE, "Unable to write to endpoint");
	}

	exit(EXIT_SUCCESS);
}

static void noreturn
initctl_system(const char *endpoint, uint8_t id) {
	const int fd = initctl_open(endpoint);

	if (write(fd, &id, sizeof (id)) != sizeof (id)) {
		err(EXIT_FAILURE, "Unable to write to endpoint");
	}

	exit(EXIT_SUCCESS);
}

static void noreturn
shutdown_main(int argc, char **argv) {
	uint8_t id = COMMAND(SYSTEM_POWEROFF);
	int c;

	while (c = getopt(argc, argv, ":HPr"), c != -1) {
		switch (c) {
		case 'H': id = COMMAND(SYSTEM_HALT);     break;
		case 'P': id = COMMAND(SYSTEM_POWEROFF); break;
		case 'r': id = COMMAND(SYSTEM_REBOOT);   break;
		case ':':
			warnx("Option -%c requires an operand\n", optopt);
			exit(EXIT_FAILURE);
		case '?':
			warnx("Unrecognized option -%c\n", optopt);
			exit(EXIT_FAILURE);
		}
	}

	initctl_system(CONFIG_ENDPOINTS_DIRECTORY, id);
}

static void noreturn
initctl_usage(const char *progname) {
	fprintf(stderr, "usage: %s ...\n", progname);
	exit(EXIT_FAILURE);
}

static void noreturn
initctl_main(int argc, char **argv) {
	const char *endpoint = CONFIG_ENDPOINTS_DIRECTORY;
	uint8_t id;
	int c;

	while (c = getopt(argc, argv, ":c:"), c != -1) {
		switch (c) {
		case 'c':
			endpoint = optarg;
			break;
		case ':':
			warnx("Option -%c requires an operand", optopt);
			initctl_usage(*argv);
		case '?':
			warnx("Unrecognized option -%c", optopt);
			initctl_usage(*argv);
		}
	}

	if (optind == argc) {
		warnx("Missing command");
		initctl_usage(*argv);
	}

	id = initctl_command_id(argv[optind]);

	if (id == COMMAND(ENDPOINT_CREATE)) {
		capset_t kept = 0;

		if (argc - optind < 3) {
			warnx("Missing arguments for endpoint creation");
			initctl_usage(*argv);
		}

		for (unsigned int i = optind + 1; i < argc; i++) {
			const char * const command = argv[i];
			const capset_t capability = 1 << initctl_command_id(command);

			if ((capability & CAPSET_ALL) == 0) {
				warnx("Invalid kept command command '%s'", command);
				initctl_usage(*argv);
			}

			kept |= capability;
		}

		initctl_endpoint_create(endpoint, id, kept ^ CAPSET_ALL, argv[optind + 1]);
	}

	if (id <= COMMAND(DAEMON_END)) {

		if (argc - optind != 2) {
			warnx("Unexpected arguments for daemon command");
			initctl_usage(*argv);
		}

		initctl_daemon(endpoint, id, argv[optind + 1]);
	}

	if (id <= COMMAND(SYSTEM_SUSPEND)) {

		if (argc - optind != 1) {
			warnx("Unexpected arguments for system command");
			initctl_usage(*argv);
		}

		initctl_system(endpoint, id);
	}

	warnx("Invalid command");
	initctl_usage(*argv);
}

int
main(int argc, char **argv) {
	static const char * const synopses[] = {
		[SYNOPSIS_HALT]     = "halt",
		[SYNOPSIS_REBOOT]   = "reboot",
		[SYNOPSIS_SUSPEND]  = "suspend",
		[SYNOPSIS_POWEROFF] = "poweroff",
		[SYNOPSIS_SHUTDOWN] = "shutdown",
	};
	enum synopsis synopsis = 0;
	uint8_t id;

	*argv = basename(*argv);
	while (synopsis < sizeof (synopses) / sizeof (*synopses) && strcmp(*argv, synopses[synopsis]) != 0) {
		synopsis++;
	}

	switch (synopsis) {
	case SYNOPSIS_HALT:     id = COMMAND(SYSTEM_HALT);     break;
	case SYNOPSIS_REBOOT:   id = COMMAND(SYSTEM_REBOOT);   break;
	case SYNOPSIS_SUSPEND:  id = COMMAND(SYSTEM_SUSPEND);  break;
	case SYNOPSIS_POWEROFF: id = COMMAND(SYSTEM_POWEROFF); break;
	case SYNOPSIS_SHUTDOWN: shutdown_main(argc, argv);
	case SYNOPSIS_INITCTL:  initctl_main(argc, argv);
	default: abort();
	}

	initctl_system(CONFIG_ENDPOINTS_DIRECTORY, id);
}
