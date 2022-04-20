/* SPDX-License-Identifier: BSD-3-Clause */
#include "socket_connection_node.h"

#include "capabilities.h"
#include "socket_endpoint_node.h"
#include "socket_switch.h"
#include "socket_node.h"
#include "configuration.h"
#include "daemon.h"
#include "log.h"

#include "config.h"

#include <stdlib.h> /* malloc, realloc, free */
#include <stdbool.h> /* bool */
#include <string.h> /* memchr */
#include <signal.h> /* sigqueue */
#include <unistd.h> /* close, read */
#include <sys/reboot.h> /* reboot, RB_POWER_OFF, ... */
#include <arpa/inet.h> /* ntohl */
#include <limits.h> /* NAME_MAX */

/** File path name buffer. */
struct namebuf {
	char *string; /**< Data. */
	size_t size; /**< Quantity of data. */
};

/** Connection message parser and executor. */
struct parser {
	capset_t capabilities; /**< Capabilities of our connection. */
	enum parser_state {
		PARSER_STATE_COMMAND,
		PARSER_STATE_ENDPOINT_CREATE_RESTRICTED,
		PARSER_STATE_ENDPOINT_CREATE_NAME,
		PARSER_STATE_DAEMON_START_NAME,
		PARSER_STATE_DAEMON_STOP_NAME,
		PARSER_STATE_DAEMON_RELOAD_NAME,
		PARSER_STATE_DAEMON_END_NAME,
		PARSER_STATE_INVALID,
	} state; /**< State of the parser */
	union {
		union {
			struct {
				uint32_t word; /**< Previously parsed bytes */
				unsigned int size; /**< How many bytes did we already parsed. */
			} restricted;
			struct {
				capset_t restricted; /**< Previously parsed restricted capabilities set. */
				struct namebuf namebuf; /**< Name of the endpoint being created. */
			} name;
		} endpoint;
		struct {
			struct namebuf namebuf; /**< Name of the daemon being referenced. */
		} daemon;
	};
};

/** Connection node, with its messages parser. */
struct socket_connection_node {
	struct socket_node super;
	struct parser parser;
};

/*******************************************
 * Filesystem path component buffer helper *
 *******************************************/

static inline void
namebuf_init(struct namebuf *namebuf) {
	namebuf->string = NULL;
	namebuf->size = 0;
}

static inline void
namebuf_deinit(struct namebuf *namebuf) {
	free(namebuf->string);
}

static inline bool
namebuf_is_terminated(const struct namebuf *namebuf) {
	return namebuf->string[namebuf->size - 1] == '\0';
}

static size_t
namebuf_fill(struct namebuf *namebuf, const char *buffer, size_t count) {
	const char * const nul = memchr(buffer, '\0', count);
	const size_t copied = nul != NULL ? (nul - buffer + 1) : count;
	const size_t newsize = namebuf->size + copied;

	if (newsize < NAME_MAX && memchr(buffer, '/', copied) == NULL) {
		char * const newstring = realloc(namebuf->string, newsize);

		if (newstring != NULL) {
			memcpy(newstring + namebuf->size, buffer, copied);
			namebuf->size = newsize;
			return copied;
		}
	}

	return 0;
}

/******************************
 * Connection commands parser *
 ******************************/

static inline void
queue_reboot(int howto) {
	const union sigval value = { .sival_int = howto };
#ifdef NDEBUG
	const pid_t pid = 1;
#else
	const pid_t pid = getpid();
#endif

	if (sigqueue(pid, SIGTERM, value) != 0) {
		log_error("socket_connection_node: sigqueue(0x%.8X): %m", howto);
	}
}

static inline void
parser_init(struct parser *parser, capset_t capabilities) {
	parser->capabilities = capabilities;
	parser->state = PARSER_STATE_COMMAND;
}

static inline void
parser_deinit(struct parser *parser) {
	switch (parser->state) {
	case PARSER_STATE_ENDPOINT_CREATE_NAME:
		namebuf_deinit(&parser->endpoint.name.namebuf);
		break;
	case PARSER_STATE_DAEMON_START_NAME:  [[fallthrough]];
	case PARSER_STATE_DAEMON_STOP_NAME:   [[fallthrough]];
	case PARSER_STATE_DAEMON_RELOAD_NAME: [[fallthrough]];
	case PARSER_STATE_DAEMON_END_NAME:
		namebuf_deinit(&parser->daemon.namebuf);
		break;
	default:
		break;
	}
}

static void
parser_feed_daemon_name(struct parser *parser, const char **bufferp, size_t *countp, void (* const action)(struct daemon *)) {
	const size_t copied = namebuf_fill(&parser->daemon.namebuf, *bufferp, *countp);

	if (copied > 0) {

		if (namebuf_is_terminated(&parser->daemon.namebuf)) {
			struct daemon * const daemon = configuration_find(parser->daemon.namebuf.string);

			if (daemon != NULL) {
				action(daemon);
			}

			parser->state = PARSER_STATE_COMMAND;
			namebuf_deinit(&parser->daemon.namebuf);
		}

		*bufferp += copied;
		*countp -= copied;
	} else {
		parser->state = PARSER_STATE_INVALID;
		namebuf_deinit(&parser->daemon.namebuf);
	}
}

static void
parser_feed(struct parser *parser, const char *buffer, size_t count) {

	while (count != 0) {
		switch (parser->state) {
		case PARSER_STATE_COMMAND:
			{ /* Parsing a command through its associated capability */
				const capset_t capability = 1 << (unsigned int)*buffer;

				if (CAPSET_HAS(parser->capabilities, capability)) {
					switch (capability) {
					case CAPABILITY_ENDPOINT_CREATE:
						parser->state = PARSER_STATE_ENDPOINT_CREATE_RESTRICTED;
						parser->endpoint.restricted.word = 0;
						parser->endpoint.restricted.size = 0;
						break;
					case CAPABILITY_DAEMON_START:
						parser->state = PARSER_STATE_DAEMON_START_NAME;
						namebuf_init(&parser->daemon.namebuf);
						break;
					case CAPABILITY_DAEMON_STOP:
						parser->state = PARSER_STATE_DAEMON_STOP_NAME;
						namebuf_init(&parser->daemon.namebuf);
						break;
					case CAPABILITY_DAEMON_RELOAD:
						parser->state = PARSER_STATE_DAEMON_RELOAD_NAME;
						namebuf_init(&parser->daemon.namebuf);
						break;
					case CAPABILITY_DAEMON_END:
						parser->state = PARSER_STATE_DAEMON_END_NAME;
						namebuf_init(&parser->daemon.namebuf);
						break;
					case CAPABILITY_SYSTEM_POWEROFF: queue_reboot(RB_POWER_OFF);   break;
					case CAPABILITY_SYSTEM_HALT:     queue_reboot(RB_HALT_SYSTEM); break;
					case CAPABILITY_SYSTEM_REBOOT:   queue_reboot(RB_AUTOBOOT);    break;
					case CAPABILITY_SYSTEM_SUSPEND:  queue_reboot(RB_SW_SUSPEND);  break;
					default: abort();
					}
				} else {
					parser->state = PARSER_STATE_INVALID;
				}

				buffer++;
				count--;
			}
			break;
		case PARSER_STATE_ENDPOINT_CREATE_RESTRICTED:
			parser->endpoint.restricted.word = (parser->endpoint.restricted.word << 8) | (unsigned char)*buffer;
			parser->endpoint.restricted.size++;
			if (parser->endpoint.restricted.size == sizeof (parser->endpoint.restricted.word)) {
				parser->state = PARSER_STATE_ENDPOINT_CREATE_NAME;
				parser->endpoint.name.restricted = ntohl(parser->endpoint.restricted.word) & parser->capabilities;
				namebuf_init(&parser->endpoint.name.namebuf);
			}
			buffer++;
			count--;
			break;
		case PARSER_STATE_ENDPOINT_CREATE_NAME:
			{ /* Create new endpoint from given name and restricted capabilities */
				const size_t copied = namebuf_fill(&parser->endpoint.name.namebuf, buffer, count);
				if (copied > 0) {

					if (namebuf_is_terminated(&parser->endpoint.name.namebuf)) {
						const capset_t capabilities = parser->capabilities ^ parser->endpoint.name.restricted;
						const char * const name = parser->endpoint.name.namebuf.string;

						if (capabilities != 0 && *name != '\0' && *name != '.') {
							struct socket_node * const snode = socket_endpoint_node_create(name, capabilities);

							if (snode != NULL) {
								socket_switch_insert(snode);
							}
						}

						parser->state = PARSER_STATE_COMMAND;
						namebuf_deinit(&parser->endpoint.name.namebuf);
					}

					buffer += copied;
					count -= copied;
				} else {
					parser->state = PARSER_STATE_INVALID;
					namebuf_deinit(&parser->endpoint.name.namebuf);
				}
			}
			break;
		case PARSER_STATE_DAEMON_START_NAME:  parser_feed_daemon_name(parser, &buffer, &count, daemon_start);  break;
		case PARSER_STATE_DAEMON_STOP_NAME:   parser_feed_daemon_name(parser, &buffer, &count, daemon_stop);   break;
		case PARSER_STATE_DAEMON_RELOAD_NAME: parser_feed_daemon_name(parser, &buffer, &count, daemon_reload); break;
		case PARSER_STATE_DAEMON_END_NAME:    parser_feed_daemon_name(parser, &buffer, &count, daemon_end);    break;
		case PARSER_STATE_INVALID:
			buffer += count;
			count = 0;
			break;
		}
	}
}

static void
socket_connection_node_operate(struct socket_node *snode) {
	struct socket_connection_node * const connection = (struct socket_connection_node *)snode;
	char buffer[CONFIG_CONNECTIONS_READ_BUFFER_SIZE];
	ssize_t readval;

	readval = read(connection->super.fd, buffer, sizeof (buffer));

	switch (readval) {
	case -1:
		log_error("socket_connection_node_operate: read: %m");
		[[fallthrough]];
	case 0:
		socket_switch_remove(&connection->super);
		return;
	}

	parser_feed(&connection->parser, buffer, readval);
}

static void
socket_connection_node_destroy(struct socket_node *snode) {
	struct socket_connection_node * const connection = (struct socket_connection_node *)snode;

	parser_deinit(&connection->parser);
	close(connection->super.fd);
	free(connection);
}

struct socket_node *
socket_connection_node_create(int fd, capset_t capabilities) {
	static const struct socket_node_class socket_connection_node_class = {
		.operate = socket_connection_node_operate,
		.destroy = socket_connection_node_destroy,
	};
	struct socket_connection_node * const connection = malloc(sizeof (*connection));

	if (connection != NULL) {
		connection->super.class = &socket_connection_node_class;
		connection->super.fd = fd;
		parser_init(&connection->parser, capabilities);
	}

	return &connection->super;
}
