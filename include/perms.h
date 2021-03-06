#ifndef PERMS_H
#define PERMS_H

#include <stdint.h>

#include "commands.h"

/**
 * Type for the bitmask holding which commands an ipc can execute
 */
typedef uint64_t perms_t;
#define PERMS_ZERO (1 << 0) /**< UNUSED */

#define PERMS_CREATE_ENDPOINT (1 << COMMAND_CREATE_ENDPOINT)

#define PERMS_DAEMON_START  (1 << COMMAND_DAEMON_START)
#define PERMS_DAEMON_STOP   (1 << COMMAND_DAEMON_STOP)
#define PERMS_DAEMON_RELOAD (1 << COMMAND_DAEMON_RELOAD)
#define PERMS_DAEMON_END    (1 << COMMAND_DAEMON_END)
#define PERMS_DAEMON_ALL    (PERMS_DAEMON_START | PERMS_DAEMON_STOP\
                            | PERMS_DAEMON_RELOAD | PERMS_DAEMON_END)

#define PERMS_SYSTEM_POWEROFF (1 << COMMAND_SYSTEM_POWEROFF)
#define PERMS_SYSTEM_HALT     (1 << COMMAND_SYSTEM_HALT)
#define PERMS_SYSTEM_REBOOT   (1 << COMMAND_SYSTEM_REBOOT)
#define PERMS_SYSTEM_SUSPEND  (1 << COMMAND_SYSTEM_SUSPEND)
#define PERMS_SYSTEM_ALL      (PERMS_SYSTEM_POWEROFF | PERMS_SYSTEM_HALT\
                              | PERMS_SYSTEM_REBOOT | PERMS_SYSTEM_SUSPEND)

#define PERMS_ALL (PERMS_CREATE_ENDPOINT | PERMS_DAEMON_ALL | PERMS_SYSTEM_ALL)

#define PERMS_AUTHORIZES(perms, commands) (((perms) | (1 << commands)) == (perms))

/* PERMS_H */
#endif
