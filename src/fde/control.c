#include "control.h"

#include "../log.h"

#include <stdlib.h>
#include <ctype.h> /* isdigit */

struct control *
control_create(void) {
	struct control *control
		= malloc(sizeof (*control));

	control->state = CONTROL_STATE_COMMAND_DETERMINATION;
	control->command = COMMAND_READER_START;

	return control;
}

void
control_destroy(struct control *control) {

	if (control->state == CONTROL_STATE_CCTL_NAME) {
		free(control->cctl.name.value);
	}

	free(control);
}

static inline void
control_state_end(struct control *control, char c) {

	control->state = CONTROL_STATE_COMMAND_DETERMINATION;
	control->command = COMMAND_READER_START;
}

static inline void
control_state_command_determination(struct control *control, char c) {

	if (c == ' ') {
		if (control->command == COMMAND_CREATE_CONTROLLER) {
			control->state = CONTROL_STATE_CCTL_REMOVING_COMMANDS;
			control->cctl.permsmask = 0;
			control->cctl.removing = COMMAND_READER_START;
		} else if (COMMAND_IS_DAEMON(control->command)
			|| COMMAND_IS_SYSTEM(control->command)) {
			control->state = CONTROL_STATE_TIME;
			control->planified.when = 0;
		} else {
			control->state = CONTROL_STATE_DISCARDING;
		}
	} else if ((control->command = command_reader_next(control->command, c))
		== COMMAND_READER_DISCARDING) {
		control->state = CONTROL_STATE_DISCARDING;
	}
}

static inline void
control_state_time(struct control *control, char c) {

	if (!isdigit(c)
		|| (control->planified.when *= 10) < 0
		|| (control->planified.when += c - '0') < 0) {
		if (c == ' ' && COMMAND_IS_DAEMON(control->command)) {
			control->state = CONTROL_STATE_DAEMON_NAME;
			control->planified.daemonhash = hash_start();
		} else if (c == '\0' && COMMAND_IS_SYSTEM(control->command)) {
			control->state = CONTROL_STATE_END;
		} else {
			control->state = CONTROL_STATE_DISCARDING;
		}
	}
}

static inline void
control_state_cctl_removing_commands(struct control *control, char c) {

	control->state = CONTROL_STATE_DISCARDING;
}

static inline void
control_state_cctl_name(struct control *control, char c) {

	control->state = CONTROL_STATE_DISCARDING;
}

static inline void
control_state_daemon_name(struct control *control, char c) {

	if (c == '\0') {
		control->state = CONTROL_STATE_END;
	} else if (c == '/') {
		control->state = CONTROL_STATE_DISCARDING;
	} else {
		control->planified.daemonhash
			= hash_update(control->planified.daemonhash, c);
	}
}

bool
control_update(struct control *control, char c) {

	if (isgraph(c) || isblank(c)) {
		log_print("On character: '%c': ", c);
	} else {
		log_print("On character: %d: ", (int)c);
	}

	switch (control->state) {
	case CONTROL_STATE_END:
		log_print("Control-state-end\n");
		control_state_end(control, c);
		/* fallthrough */
	case CONTROL_STATE_COMMAND_DETERMINATION:
		log_print("Control-state-command-determination\n");
		control_state_command_determination(control, c);
		break;
	case CONTROL_STATE_TIME:
		log_print("Control-state-time\n");
		control_state_time(control, c);
		break;
	case CONTROL_STATE_CCTL_REMOVING_COMMANDS:
		log_print("Control-state-cctl-removing-commands\n");
		control_state_cctl_removing_commands(control, c);
		break;
	case CONTROL_STATE_CCTL_NAME:
		log_print("Control-state-cctl-name\n");
		control_state_cctl_name(control, c);
		break;
	case CONTROL_STATE_DAEMON_NAME:
		log_print("Control-state-daemon-name\n");
		control_state_daemon_name(control, c);
		break;
	default: /* CONTROL_STATE_DISCARDING */
		log_print("Control-state-discarding\n");
		if (c == '\0') {
			control->state = CONTROL_STATE_END;
			control->command = COMMAND_UNDEFINED;
		} break;
	}

	return control->state == CONTROL_STATE_END;
}

