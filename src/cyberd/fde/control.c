#include "control.h"

#include "../log.h"

#include "config.h"

#include <stdlib.h>
#include <ctype.h> /* isdigit */

struct control *
control_create(void) {
	struct control *control
		= malloc(sizeof(*control));

	if(control != NULL) {
		control->state = CONTROL_STATE_COMMAND_DETERMINATION;
		control->command = COMMAND_READER_START;
	}

	return control;
}

void
control_destroy(struct control *control) {

	if(control->state == CONTROL_STATE_CCTL_NAME
		|| (control->state == CONTROL_STATE_END
			&& control->command == COMMAND_CREATE_CONTROLLER)) {
		free(control->cctl.name.value);
	}

	free(control);
}

static inline void
control_state_end(struct control *control, char c) {

	if(control->command == COMMAND_CREATE_CONTROLLER) {
		free(control->cctl.name.value);
	}

	control->state = CONTROL_STATE_COMMAND_DETERMINATION;
	control->command = COMMAND_READER_START;
}

static inline void
control_state_command_determination(struct control *control, char c) {

	if(c == ' ') {
		if(control->command == COMMAND_CREATE_CONTROLLER) {
			control->state = CONTROL_STATE_CCTL_REMOVING_COMMANDS;
			control->cctl.permsmask = 0;
			control->cctl.removing = COMMAND_READER_START;
		} else if(COMMAND_IS_DAEMON(control->command)
			|| COMMAND_IS_SYSTEM(control->command)) {
			control->state = CONTROL_STATE_TIME;
			control->planified.when = 0;
		} else {
			control->state = CONTROL_STATE_DISCARDING;
		}
	} else if((control->command = command_reader_next(control->command, c))
		== COMMAND_READER_DISCARDING) {
		control->state = CONTROL_STATE_DISCARDING;
	}
}

static inline void
control_state_time(struct control *control, char c) {

	if(!isdigit(c)
		|| (control->planified.when *= 10) < 0
		|| (control->planified.when += c - '0') < 0) {
		if(c == ' ' && COMMAND_IS_DAEMON(control->command)) {
			control->state = CONTROL_STATE_DAEMON_NAME;
			control->planified.daemonhash = hash_start();
		} else if(c == '\0' && COMMAND_IS_SYSTEM(control->command)) {
			control->state = CONTROL_STATE_END;
		} else {
			control->state = CONTROL_STATE_DISCARDING;
		}
	}
}

static inline void
control_state_cctl_removing_commands(struct control *control, char c) {

	if(c == ' ') {
		if(COMMAND_IS_VALID(control->cctl.removing)) {
			control->cctl.permsmask |= 1 << control->cctl.removing;
			control->cctl.removing = COMMAND_READER_START;
		} else {
			control->state = CONTROL_STATE_DISCARDING;
		}
	} else if(c == '\t') {

		control->state = CONTROL_STATE_CCTL_NAME;
		control->cctl.name.value = calloc(8, sizeof(char));
		control->cctl.name.length = 0;
		control->cctl.name.capacity = 8;
	} else {
		enum command_reader removing
			= command_reader_next(control->cctl.removing, c);

		if(removing == COMMAND_READER_DISCARDING) {
			control->state = CONTROL_STATE_DISCARDING;
		} else {
			control->cctl.removing = removing;
		}
	}
}

static inline void
control_state_cctl_name(struct control *control, char c) {

	if(c == '\0') {
		control->cctl.name.value[control->cctl.name.length] = '\0';
		control->state = CONTROL_STATE_END;
	} else if((control->cctl.name.length += 1) <= CONFIG_MAX_ACCEPTOR_LEN) {

		if(control->cctl.name.length == control->cctl.name.capacity) {
			control->cctl.name.value = realloc(control->cctl.name.value,
				(control->cctl.name.capacity <<= 1) * sizeof(char));
		}

		control->cctl.name.value[control->cctl.name.length - 1] = c;
	} else {
		free(control->cctl.name.value);
		control->state = CONTROL_STATE_DISCARDING;
	}
}

static inline void
control_state_daemon_name(struct control *control, char c) {

	if(c == '\0') {
		control->state = CONTROL_STATE_END;
	} else if(c == '/') {
		control->state = CONTROL_STATE_DISCARDING;
	} else {
		control->planified.daemonhash
			= hash_update(control->planified.daemonhash, c);
	}
}

bool
control_update(struct control *control, char c) {

	switch(control->state) {
	case CONTROL_STATE_END:
		control_state_end(control, c);
		/* fallthrough */
	case CONTROL_STATE_COMMAND_DETERMINATION:
		control_state_command_determination(control, c);
		break;
	case CONTROL_STATE_TIME:
		control_state_time(control, c);
		break;
	case CONTROL_STATE_CCTL_REMOVING_COMMANDS:
		control_state_cctl_removing_commands(control, c);
		break;
	case CONTROL_STATE_CCTL_NAME:
		control_state_cctl_name(control, c);
		break;
	case CONTROL_STATE_DAEMON_NAME:
		control_state_daemon_name(control, c);
		break;
	default: /* CONTROL_STATE_DISCARDING */
		if(c == '\0') {
			control->state = CONTROL_STATE_END;
			control->command = COMMAND_UNDEFINED;
		} break;
	}

	return control->state == CONTROL_STATE_END;
}

