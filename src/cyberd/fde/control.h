#ifndef CONTROL_H
#define CONTROL_H

#include "perms.h"
#include "command_reader.h"

#include <stdbool.h>
#include <sys/types.h>

/**
 * Structure representing a dynamic string for use in the automaton
 */
struct control_name {
	unsigned int capacity; /**< Capacity, in bytes, of the buffer */
	unsigned int length;   /**< Current length of the string, always less than capacity */
	char *value;           /**< string buffer, not NUL terminated until last input */
};

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
				struct control_name name; /**< Valid in CONTROL_STATE_CCTL_NAME, take care of potential memory leaks */
			};
		} cctl; /**< Valid in CONTROL_STATE_CCTL_REMOVING_COMMANDS and CONTROL_STATE_CCTL_NAME */
		struct {
			time_t when;
			struct control_name daemon;
		} planified; /**< Valid in CONTROL_STATE_TIME and CONTROL_STATE_DAEMON_NAME, take care of potential memory leaks */
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
