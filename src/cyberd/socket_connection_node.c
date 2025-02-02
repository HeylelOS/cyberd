/* SPDX-License-Identifier: BSD-3-Clause */
#include "socket_connection_node.h"

#include "capabilities.h"
#include "socket_endpoint_node.h"
#include "socket_switch.h"
#include "socket_node.h"
#include "configuration.h"
#include "daemon.h"

#include <stdlib.h> /* malloc, realloc, free */
#include <string.h> /* memchr */
#include <signal.h> /* sigqueue */
#include <syslog.h> /* syslog */
#include <unistd.h> /* close, read */
#include <sys/reboot.h> /* reboot, RB_POWER_OFF, ... */
#include <arpa/inet.h> /* ntohl */
#include <limits.h> /* NAME_MAX */

/** Connection message parser and executor. */
struct parser {
	capset_t capabilities; /**< Capabilities of our connection. */
	enum parser_state {
		PARSER_STATE_COMMAND,
		PARSER_STATE_ENDPOINT_CREATE_CAPABILITIES,
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
				unsigned int size; /**< How many bytes already parsed. */
			} capabilities;
			struct {
				capset_t capabilities; /**< Previously parsed capabilities set. */
				unsigned int len; /**< Current Length of name. */
				char buf[NAME_MAX + 1]; /**< Name of the endpoint being created. */
			} name;
		} endpoint;
		struct {
			unsigned int len; /**< Current Length of name. */
			char buf[NAME_MAX + 1]; /**< Name of the daemon being referenced. */
		} daemon;
	};
};

/** Connection node, with its messages parser. */
struct socket_connection_node {
	struct socket_node super;
	struct parser parser;
};

/******************************
 * Connection commands parser *
 ******************************/

static inline void
queue_reboot(int howto) {
	const union sigval value = { .sival_int = howto };

	if (sigqueue(getpid(), SIGTERM, value) != 0) {
		syslog(LOG_ERR, "socket_connection_node: sigqueue(%#.8x): %m", howto);
	}
}

static void
parser_feed_command(struct parser *parser, capset_t capability) {

	if (CAPSET_HAS(parser->capabilities, capability)) {
		switch (capability) {
		case CAPABILITY_ENDPOINT_CREATE:
			parser->state = PARSER_STATE_ENDPOINT_CREATE_CAPABILITIES;
			parser->endpoint.capabilities.word = 0;
			parser->endpoint.capabilities.size = 0;
			break;
		case CAPABILITY_DAEMON_START:
			parser->state = PARSER_STATE_DAEMON_START_NAME;
			parser->daemon.len = 0;
			break;
		case CAPABILITY_DAEMON_STOP:
			parser->state = PARSER_STATE_DAEMON_STOP_NAME;
			parser->daemon.len = 0;
			break;
		case CAPABILITY_DAEMON_RELOAD:
			parser->state = PARSER_STATE_DAEMON_RELOAD_NAME;
			parser->daemon.len = 0;
			break;
		case CAPABILITY_DAEMON_END:
			parser->state = PARSER_STATE_DAEMON_END_NAME;
			parser->daemon.len = 0;
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
}

static int
name_fill(const char **inp, size_t *inlenp, char *out, unsigned int *outlenp) {
	const char * const in = *inp;
	const size_t inlen = *inlenp;
	const char * const nul = memchr(in, '\0', inlen);
	const size_t len = nul != NULL ? (nul - in + 1) : inlen;
	const size_t outlen = *outlenp + len;

	if (outlen > NAME_MAX + 1 || memchr(in, '/', len) != NULL) {
		return -1;
	}

	memcpy(out + *outlenp, in, len);
	*outlenp = outlen;
	*inlenp += len;
	*inp += len;

	return len;
}

static void
parser_feed_endpoint_create_name(struct parser *parser, const char **bufferp, size_t *countp) {

	if (name_fill(bufferp, countp, parser->endpoint.name.buf, &parser->endpoint.name.len) < 0) {
		parser->state = PARSER_STATE_INVALID;
		return;
	}

	if (parser->endpoint.name.buf[parser->endpoint.name.len - 1] == '\0') {
		const capset_t capabilities = parser->capabilities & parser->endpoint.name.capabilities;
		const char * const name = parser->endpoint.name.buf;

		if (capabilities != 0 && *name != '\0' && *name != '.') {
			struct socket_node * const snode = socket_endpoint_node_create(name, capabilities);

			if (snode != NULL) {
				socket_switch_insert(snode);
			}
		}
		parser->state = PARSER_STATE_COMMAND;
	}
}

static void
parser_feed_daemon_name(struct parser *parser, const char **bufferp, size_t *countp, void (* const action)(struct daemon *)) {

	if (name_fill(bufferp, countp, parser->daemon.buf, &parser->daemon.len) < 0) {
		parser->state = PARSER_STATE_INVALID;
		return;
	}

	if (parser->daemon.buf[parser->daemon.len - 1] == '\0') {
		struct daemon * const daemon = configuration_find(parser->daemon.buf);

		if (daemon != NULL) {
			action(daemon);
		}

		parser->state = PARSER_STATE_COMMAND;
	}
}

static void
parser_feed(struct parser *parser, const char *buffer, size_t count) {

	while (count != 0) {
		switch (parser->state) {
		case PARSER_STATE_COMMAND:
			parser_feed_command(parser, (capset_t)1 << (unsigned int)*buffer);
			buffer++;
			count--;
			break;
		case PARSER_STATE_ENDPOINT_CREATE_CAPABILITIES:
			parser->endpoint.capabilities.word = (parser->endpoint.capabilities.word << 8) | (unsigned char)*buffer;
			parser->endpoint.capabilities.size++;
			if (parser->endpoint.capabilities.size == sizeof (parser->endpoint.capabilities.word)) {
				parser->state = PARSER_STATE_ENDPOINT_CREATE_NAME;
				parser->endpoint.name.capabilities = ntohl(parser->endpoint.capabilities.word);
				parser->endpoint.name.len = 0;
			}
			buffer++;
			count--;
			break;
		case PARSER_STATE_ENDPOINT_CREATE_NAME:
			parser_feed_endpoint_create_name(parser, &buffer, &count);
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
	char buffer[CONFIG_SOCKET_CONNECTIONS_BUFFER_SIZE];

	const ssize_t readval = read(connection->super.fd, buffer, sizeof (buffer));
	switch (readval) {
	case -1:
		syslog(LOG_ERR, "socket_connection_node_operate: read: %m");
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

	if (connection == NULL) {
		return NULL;
	}

	connection->super.class = &socket_connection_node_class;
	connection->super.fd = fd;
	connection->parser.capabilities = capabilities;
	connection->parser.state = PARSER_STATE_COMMAND;

	return &connection->super;
}
