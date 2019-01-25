#ifndef AUTOMATA_H
#define AUTOMATA_H

/******************************************************************
 This file defines automata used to recognize expressions during
 inter process communication. Note src/configuration.c may use 
 automaton_filename but doesn't as it only checks for the first '.'
*******************************************************************/

#include "commands.h"

#include <sys/types.h>

struct automaton_filename {
	size_t length;
	enum automaton_filename_state {
		AUTOMATON_FILENAME_START,
		AUTOMATON_FILENAME_VALID,
		AUTOMATON_FILENAME_INVALID
	} state;
};

#define AUTOMATON_FILENAME_STATE_VALID(s) ((s) == AUTOMATON_FILENAME_VALID)

struct automaton_time {
	time_t time;
	enum automaton_time_state {
		AUTOMATON_TIME_VALID,
		AUTOMATON_TIME_INVALID
	} state;
};

#define AUTOMATON_TIME_STATE_VALID(s) ((s) == AUTOMATON_TIME_VALID)

struct automaton_command {
	enum automaton_command_state {
		AUTOMATON_COMMAND_START,
		/* commands states */
		AUTOMATON_COMMAND_CREATE_CONTROL = COMMAND_CREATE_CONTROL,
		AUTOMATON_COMMAND_DAEMON_START = COMMAND_DAEMON_START,
		AUTOMATON_COMMAND_DAEMON_STOP = COMMAND_DAEMON_STOP,
		AUTOMATON_COMMAND_DAEMON_RELOAD = COMMAND_DAEMON_RELOAD,
		AUTOMATON_COMMAND_DAEMON_END = COMMAND_DAEMON_END,
		AUTOMATON_COMMAND_SYSTEM_HALT = COMMAND_SYSTEM_HALT,
		AUTOMATON_COMMAND_SYSTEM_REBOOT = COMMAND_SYSTEM_REBOOT,
		AUTOMATON_COMMAND_SYSTEM_SLEEP = COMMAND_SYSTEM_SLEEP,
		/* partial states */
		AUTOMATON_COMMAND_D,
		AUTOMATON_COMMAND_DS,
		AUTOMATON_COMMAND_DST,
		AUTOMATON_COMMAND_DR,
		AUTOMATON_COMMAND_DRL,
		AUTOMATON_COMMAND_DE,
		AUTOMATON_COMMAND_DEN,
		AUTOMATON_COMMAND_S,
		AUTOMATON_COMMAND_SH,
		AUTOMATON_COMMAND_SHL,
		AUTOMATON_COMMAND_SR,
		AUTOMATON_COMMAND_SRB,
		AUTOMATON_COMMAND_SS,
		AUTOMATON_COMMAND_SSL,
		AUTOMATON_COMMAND_C,
		AUTOMATON_COMMAND_CC,
		AUTOMATON_COMMAND_CCT,
		/* invalid state */
		AUTOMATON_COMMAND_INVALID
	} state;
};

#define AUTOMATON_COMMAND_STATE_VALID(s) ((s) >= AUTOMATON_COMMAND_CREATE_CONTROL\
									&& (s) <= AUTOMATON_COMMAND_SYSTEM_SLEEP)

void
automaton_filename_init(struct automaton_filename *a);
void
automaton_filename_run(struct automaton_filename *a, char c);

void
automaton_time_init(struct automaton_time *a);
void
automaton_time_run(struct automaton_time *a, char c);

void
automaton_command_init(struct automaton_command *a);
void
automaton_command_run(struct automaton_command *a, char c);

/* AUTOMATA_H */
#endif
