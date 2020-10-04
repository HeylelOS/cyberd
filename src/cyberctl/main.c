#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <err.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "config.h"
#include "commands.h"
#include "perms.h"

struct cyberctl_args {
	const char *endpoint;
	int64_t when;
};

struct command_identifier {
	int id;
	char *name;
} const commands[] = {
	{ .id = COMMAND_CREATE_ENDPOINT, .name = "create-endpoint" },
	{ .id = COMMAND_DAEMON_START, .name = "start" },
	{ .id = COMMAND_DAEMON_STOP, .name = "stop" },
	{ .id = COMMAND_DAEMON_RELOAD, .name = "reload" },
	{ .id = COMMAND_DAEMON_END, .name = "end" },
	{ .id = COMMAND_SYSTEM_POWEROFF, .name = "poweroff" },
	{ .id = COMMAND_SYSTEM_HALT, .name = "halt" },
	{ .id = COMMAND_SYSTEM_REBOOT, .name = "reboot" },
	{ .id = COMMAND_SYSTEM_SUSPEND, .name = "suspend" },
};

static int
cyberctl_command_find_id(const char *name) {
	const struct command_identifier *current = commands,
		* const end = commands + sizeof(commands) / sizeof(*commands);

	while(current != end && strcmp(current->name, name) != 0) {
		current++;
	}

	return current != end ? current->id : COMMAND_UNDEFINED;
}

static int
cyberctl_endpoint_open(const char *name) {
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);

	if(fd != -1) {
		struct sockaddr_un addr = { .sun_family = AF_LOCAL };

		if(snprintf(addr.sun_path, sizeof(addr.sun_path),
			CONFIG_ENDPOINTS_DIRECTORY"/%s", name) >= sizeof(addr.sun_path)) {
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
cyberctl_endpoint_close(int fd) {
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

static int
cyberctl_create_endpoint(int fd, uint8_t id, char **argpos, char **argend) {
	if(argpos == argend) {
		warnx("Missing argument for create-endpoint command");
		return EXIT_FAILURE;
	}

	const char *newendpoint = *argpos;
	size_t newendpointlen = strlen(newendpoint);
	uint8_t buffer[sizeof(uint8_t) + sizeof(uint64_t) + newendpointlen + 1];
	perms_t *permsp = (perms_t *)(buffer + 1);
	char *name = (char *)(permsp + 1);

	buffer[0] = id;

	while(++argpos != argend) {
		*permsp |= cyberctl_command_find_id(*argpos);
	}

	*permsp = htonll(*permsp);

	memcpy(name, newendpoint, newendpointlen + 1);

	if(write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
		warn("cyberctl_create_endpoint: write");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
cyberctl_daemon(int fd, uint8_t id, char **argpos, char **argend, const struct cyberctl_args *args) {
	if(argpos == argend) {
		warnx("Missing arguments for daemon command");
		return EXIT_FAILURE;
	}

	while(argpos != argend) {
		char *daemon = *argpos;
		size_t daemonlen = strlen(daemon);
		uint8_t buffer[sizeof(uint8_t) + sizeof(uint64_t) + daemonlen + 1];
		uint64_t *whenp = (uint64_t *)(buffer + 1);
		char *name = (char *)(whenp + 1);

		buffer[0] = id;
		*whenp = htonll((uint64_t)args->when);
		memcpy(name, daemon, daemonlen + 1);

		if(write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
			warn("cyberctl_daemon: write");
			return EXIT_FAILURE;
		}

		argpos++;
	}

	return EXIT_SUCCESS;
}

static int
cyberctl_system(int fd, uint8_t id, char **argpos, char **argend, const struct cyberctl_args *args) {
	if(argpos != argend) {
		warnx("Invalid arguments for system command");
		return EXIT_FAILURE;
	}

	uint8_t buffer[sizeof(uint8_t) + sizeof(uint64_t)];
	uint64_t *whenp = (uint64_t *)(buffer + 1);

	buffer[0] = id;
	*whenp = htonll((uint64_t)args->when);

	if(write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
		warn("cyberctl_system: write");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static void
cyberctl_usage(const char *cyberctlname) {
	fprintf(stderr, "usage: %s [-s endpoint] [-E] [-t when] <start|stop|reload|end> daemon...\n"
		"       %s [-s endpoint] [-E] [-t when] <poweroff|halt|reboot|suspend>\n"
		"       %s [-s endpoint] <create-endpoint> [start|stop|reload|end|poweroff|halt|reboot|suspend|create-endpoint]...\n",
		cyberctlname, cyberctlname, cyberctlname);
	exit(EXIT_FAILURE);
}

static struct cyberctl_args
cyberctl_parse_args(int argc, char **argv) {
	struct cyberctl_args args = {
		.endpoint = CONFIG_ENDPOINT_ROOT,
		.when = 0,
	};
	bool epoch = false;
	int c;

	while((c = getopt(argc, argv, ":s:t:E")) != -1) {
		switch(c) {
		case 's':
			args.endpoint = optarg;
			break;
		case 't': {
			char *end;
			long value = strtol(optarg, &end, 10);

			if(*end == '\0' && *optarg != '\0') {
				args.when = (time_t) value;
			} else {
				warnx("Expected integral value for -%c, found '%s' (stopped at '%c')", optopt, optarg, *end);
				cyberctl_usage(*argv);
			}
		} break;
		case 'E':
			epoch = true;
			break;
		case '?':
			warnx("Invalid option -%c", optopt);
			cyberctl_usage(*argv);
		case ':':
			warnx("Missing option argument after -%c", optopt);
			cyberctl_usage(*argv);
		}
	}

	if(!epoch) {
		args.when += time(NULL);
	}

	if(optind == argc) {
		warnx("Missing command name");
		cyberctl_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct cyberctl_args args = cyberctl_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;
	int fd = cyberctl_endpoint_open(args.endpoint);
	int id = cyberctl_command_find_id(*argpos);
	int retval = EXIT_FAILURE;

	if(id == COMMAND_CREATE_ENDPOINT) {
		retval = cyberctl_create_endpoint(fd, id, argpos + 1, argend);
	} else if(COMMAND_IS_DAEMON(id)) {
		retval = cyberctl_daemon(fd, id, argpos + 1, argend, &args);
	} else if(COMMAND_IS_SYSTEM(id)) {
		retval = cyberctl_system(fd, id, argpos + 1, argend, &args);
	} else {
		warnx("Invalid command name: %s", *argpos);
	}

	cyberctl_endpoint_close(fd);

	return retval;
}

