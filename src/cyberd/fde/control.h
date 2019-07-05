#ifndef CONTROL_H
#define CONTROL_H

#include "perms.h"
#include "command_reader.h"
#include "../hash.h"

#include <stdbool.h>
#include <sys/types.h>

/**
 * This structure is used by File Descriptor Element of type controller
 * to determine which command to execute and its arguments
 */
struct control {
	enum control_state {
		CONTROL_STATE_COMMAND_DETERMINATION,
		CONTROL_STATE_TIME,
		CONTROL_STATE_CCTL_REMOVING_COMMANDS,
		CONTROL_STATE_CCTL_NAME,
		CONTROL_STATE_DAEMON_NAME,
		CONTROL_STATE_END,
		CONTROL_STATE_DISCARDING
	} state; /**< Current state of command reading */

	enum command_reader command; /**< What command is decided? Valid after a successfull command determination */

	union {
		struct {
			perms_t permsmask; /**< Mask of which command to remove if we create a controller */
			union {
				enum command_reader removing; /**< Valid in CONTROL_STATE_CCTL_REMOVING_COMMANDS */
				struct {
					char *value;
					size_t capacity;
					size_t length;
				} name; /**< Valid in CONTROL_STATE_CCTL_NAME, take care of potential memory leaks */
			};
		} cctl; /**< Valid in CONTROL_STATE_CCTL_REMOVING_COMMANDS and CONTROL_STATE_CCTL_NAME */
		struct {
			time_t when;
			hash_t daemonhash;
		} planified; /**< Valid in CONTROL_STATE_TIME and CONTROL_STATE_DAEMON_NAME */
	};
};

/**
 * Create a new control structure
 * @return Newly allocated struct control, must be control_destroy()'d
 */
struct control *
control_create(void);

/**
 * Destroys a struct control
 * @param control A previously control_create()'d struct control
 */
void
control_destroy(struct control *control);

/**
 * Updates the stage in command reading
 * @param control The struct control holding the reading progress
 * @param c The read character
 * @return Whether a command has ended reading, or discarding stopped
 */
bool
control_update(struct control *control, char c);

/* CONTROL_H */
#endif
