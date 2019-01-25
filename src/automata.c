#include "automata.h"

#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

#define DIRENT_NAME_MAX (sizeof (((struct dirent *)NULL)->d_name))

void
automaton_filename_init(struct automaton_filename *a) {
	a->length = 0;
	a->state = AUTOMATON_FILENAME_START;
}

void
automaton_filename_run(struct automaton_filename *a,
	char c) {

	switch (a->state) {
	case AUTOMATON_FILENAME_START:
		if (c == '.') {
			a->state = AUTOMATON_FILENAME_INVALID;
			break;
		}
		a->state = AUTOMATON_FILENAME_VALID;
		/* fallthrough */
	case AUTOMATON_FILENAME_VALID:
		if (c == '\0' || c == '/'
			|| (a->length += 1) >= DIRENT_NAME_MAX) {
			/* Standard doesn't specify minimal limit for d_name,
				only maximum one as NAME_MAX */
			a->state = AUTOMATON_FILENAME_INVALID;
		}
		break;
	default: /* AUTOMATON_FILENAME_INVALID */
		break;
	}
}

void
automaton_time_init(struct automaton_time *a) {
	a->time = 0;
	a->state = AUTOMATON_TIME_VALID;
}

void
automaton_time_run(struct automaton_time *a,
	char c) {

	if (a->state == AUTOMATON_TIME_VALID
		&& a->time >= 0) {

		if (!isdigit(c)
			|| (a->time *= 10) < 0
			|| (a->time += c - '0') < 0) {
			a->state = AUTOMATON_TIME_INVALID;
		}
	}
}

void
automaton_command_init(struct automaton_command *a) {
	a->state = AUTOMATON_COMMAND_START;
}

void
automaton_command_run(struct automaton_command *a,
	char c) {

	switch (a->state) {
	case AUTOMATON_COMMAND_START:
		if (c == 'd') {
			a->state = AUTOMATON_COMMAND_D;
		} else if (c == 's') {
			a->state = AUTOMATON_COMMAND_S;
		} else if (c == 'c') {
			a->state = AUTOMATON_COMMAND_C;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_D:
		if (c == 's') {
			a->state = AUTOMATON_COMMAND_DS;
		} else if (c == 'r') {
			a->state = AUTOMATON_COMMAND_DR;
		} else if (c == 'e') {
			a->state = AUTOMATON_COMMAND_DE;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_DS:
		if (c == 't') {
			a->state = AUTOMATON_COMMAND_DST;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_DST:
		if (c == 't') {
			a->state = AUTOMATON_COMMAND_DAEMON_START;
		} else if (c == 'p') {
			a->state = AUTOMATON_COMMAND_DAEMON_STOP;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_DR:
		if (c == 'l') {
			a->state = AUTOMATON_COMMAND_DRL;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_DRL:
		if (c == 'd') {
			a->state = AUTOMATON_COMMAND_DAEMON_RELOAD;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_DE:
		if (c == 'n') {
			a->state = AUTOMATON_COMMAND_DEN;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_DEN:
		if (c == 'd') {
			a->state = AUTOMATON_COMMAND_DAEMON_END;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_S:
		if (c == 'h') {
			a->state = AUTOMATON_COMMAND_SH;
		} else if (c == 's') {
			a->state = AUTOMATON_COMMAND_SS;
		} else if (c == 'r') {
			a->state = AUTOMATON_COMMAND_SR;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_SH:
		if (c == 'l') {
			a->state = AUTOMATON_COMMAND_SHL;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_SHL:
		if (c == 't') {
			a->state = AUTOMATON_COMMAND_SYSTEM_HALT;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_SR:
		if (c == 'b') {
			a->state = AUTOMATON_COMMAND_SRB;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_SRB:
		if (c == 't') {
			a->state = AUTOMATON_COMMAND_SYSTEM_REBOOT;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_SS:
		if (c == 'l') {
			a->state = AUTOMATON_COMMAND_SSL;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_SSL:
		if (c == 'p') {
			a->state = AUTOMATON_COMMAND_SYSTEM_SLEEP;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_C:
		if (c == 'c') {
			a->state = AUTOMATON_COMMAND_CC;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_CC:
		if (c == 't') {
			a->state = AUTOMATON_COMMAND_CCT;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	case AUTOMATON_COMMAND_CCT:
		if (c == 'l') {
			a->state = AUTOMATON_COMMAND_CREATE_CONTROL;
		} else {
			a->state = AUTOMATON_COMMAND_INVALID;
		}
		break;
	default: /* valid commands, or invalid state */
		a->state = AUTOMATON_COMMAND_INVALID;
		break;
	}
}

