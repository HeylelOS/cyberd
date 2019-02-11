#ifndef CONTROL_H
#define CONTROL_H

#include "perms.h"
#include "command_reader.h"
#include "../hash.h"

#include <stdbool.h>
#include <sys/types.h>

struct control {
	enum control_state {
		CONTROL_STATE_COMMAND_DETERMINATION,
		CONTROL_STATE_TIME,
		CONTROL_STATE_CCTL_REMOVING_COMMANDS,
		CONTROL_STATE_CCTL_NAME,
		CONTROL_STATE_DAEMON_NAME,
		CONTROL_STATE_END,
		CONTROL_STATE_DISCARDING
	} state;
	enum command_reader command;
	union {
		struct {
			perms_t permsmask;
			union {
				enum command_reader removing;
				struct {
					char *value;
					size_t capacity;
					size_t length;
				} name;
			};
		} cctl;
		struct {
			time_t when;
			hash_t daemonhash;
		} planified;
	};
};

struct control *
control_create(void);

void
control_destroy(struct control *control);

bool
control_run(struct control *control, char c);

/* CONTROL_H */
#endif
