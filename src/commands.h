#ifndef COMMANDS_H
#define COMMANDS_H

/*
 * This file unifies id's given to identify a command
 * in the whole codebase, this allows direct casts and though
 * avoids tables to transform one enum into another
 */

#define COMMAND_UNDEFINED         0
/** Create a new IPC socket with less or equal rights */
#define COMMAND_CREATE_CONTROLLER 1

/** Starts a daemon if not started, stopping */
#define COMMAND_DAEMON_START      8
/** Stops a daemon (sent termination signal) if not stopping */
#define COMMAND_DAEMON_STOP       9
/** Sends a reload signal to the daemon */
#define COMMAND_DAEMON_RELOAD     10
/** Ends a signal by sending SIGKILL to it */
#define COMMAND_DAEMON_END        11

/** Ends cyberd and poweroffs system */
#define COMMAND_SYSTEM_POWEROFF   16
/** Ends cyberd and halts system */
#define COMMAND_SYSTEM_HALT       17
/** Ends cyberd and reboots system */
#define COMMAND_SYSTEM_REBOOT     18
/** Suspends system */
#define COMMAND_SYSTEM_SUSPEND    19

/** Checks if a command id is related to daemons */
#define COMMAND_IS_DAEMON(c) ((c) >= COMMAND_DAEMON_START && (c) <= COMMAND_DAEMON_END)
/** Checks if a command id is related to system */
#define COMMAND_IS_SYSTEM(c) ((c) >= COMMAND_SYSTEM_POWEROFF && (c) <= COMMAND_SYSTEM_SUSPEND)
/** Checks if a command id is valid */
#define COMMAND_IS_VALID(c) ((c) == COMMAND_CREATE_CONTROLLER\
							|| COMMAND_IS_DAEMON(c)\
							|| COMMAND_IS_SYSTEM(c))

/* COMMANDS_H */
#endif
