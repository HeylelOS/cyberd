#ifndef FDE_H
#define FDE_H

#include "automata.h"

#include <stdbool.h>

/* File Descriptor Element, for dispatcher */

typedef unsigned long fde_perms_t;
#define FDE_PERM_ACCEPTOR       (1 << 0)
#define FDE_PERM_CREATE_CONTROL (1 << COMMAND_CREATE_CONTROL)

#define FDE_PERM_DAEMON_START  (1 << COMMAND_DAEMON_START)
#define FDE_PERM_DAEMON_STOP   (1 << COMMAND_DAEMON_STOP)
#define FDE_PERM_DAEMON_RELOAD (1 << COMMAND_DAEMON_RELOAD)
#define FDE_PERM_DAEMON_END    (1 << COMMAND_DAEMON_END)
#define FDE_PERM_DAEMON_ALL    (FDE_PERM_DAEMON_START | FDE_PERM_DAEMON_STOP\
                               | FDE_PERM_DAEMON_RELOAD | FDE_PERM_DAEMON_END)

#define FDE_PERM_SYSTEM_HALT   (1 << COMMAND_SYSTEM_HALT)
#define FDE_PERM_SYSTEM_REBOOT (1 << COMMAND_SYSTEM_REBOOT)
#define FDE_PERM_SYSTEM_SLEEP  (1 << COMMAND_SYSTEM_SLEEP)
#define FDE_PERM_SYSTEM_ALL    (FDE_PERM_SYSTEM_HALT | FDE_PERM_SYSTEM_REBOOT\
                               | FDE_PERM_SYSTEM_SLEEP)

struct fd_element {
	fde_perms_t perms;
	int fd;
	struct fde_connection {
		enum fde_connection_states {
			FDE_CONNECTION_START,
			FDE_CONNECTION_TYPE,
			FDE_CONNECTION_WHEN,
			FDE_CONNECTION_ARGUMENTS,
			FDE_CONNECTION_VALID,
			FDE_CONNECTION_DISCARDING
		} state;
		int command;
		time_t when;
		fde_perms_t newperms;
		char *name;
		size_t namecap;
		union {
			struct automaton_command command;
			struct automaton_time time;
			struct automaton_filename filename;
		} automata;
	} control;
};

struct fd_element *
fde_create_acceptor(const char *path, fde_perms_t perms);

struct fd_element *
fde_create_connection(const struct fd_element *fde);

bool
fde_connection_control(struct fd_element *fde,
	char c);

void
fde_destroy(struct fd_element *fde);

/* FDE_H */
#endif
