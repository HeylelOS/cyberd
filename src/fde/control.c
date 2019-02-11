#include "control.h"

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

bool
control_run(struct control *control, char c) {

	switch (control->state) {
	case CONTROL_STATE_END:
		control->state = CONTROL_STATE_COMMAND_DETERMINATION;
		control->command = COMMAND_READER_START;
		/* fallthrough */
	case CONTROL_STATE_COMMAND_DETERMINATION:
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
		} break;
	case CONTROL_STATE_TIME:
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
		} break;
	case CONTROL_STATE_CCTL_REMOVING_COMMANDS:
		control->state = CONTROL_STATE_DISCARDING;
		break;
	case CONTROL_STATE_CCTL_NAME:
		control->state = CONTROL_STATE_DISCARDING;
		break;
	case CONTROL_STATE_DAEMON_NAME:
		if (c == '\0') {
			control->state = CONTROL_STATE_END;
		} else if (c == '/') {
			control->state = CONTROL_STATE_DISCARDING;
		} else {
			control->planified.daemonhash
				= hash_run(control->planified.daemonhash, c);
		}
	default: /* CONTROL_STATE_DISCARDING */
		if (c == '\0') {
			control->state = CONTROL_STATE_END;
		} break;
	}

	return control->state == CONTROL_STATE_END;
}

