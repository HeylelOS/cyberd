/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include <stdint.h> /* uint32_t */

#define CAPABILITY_ENDPOINT_CREATE (1 << 0)
#define CAPABILITY_DAEMON_START    (1 << 1)
#define CAPABILITY_DAEMON_STOP     (1 << 2)
#define CAPABILITY_DAEMON_RELOAD   (1 << 3)
#define CAPABILITY_DAEMON_END      (1 << 4)
#define CAPABILITY_SYSTEM_POWEROFF (1 << 5)
#define CAPABILITY_SYSTEM_HALT     (1 << 6)
#define CAPABILITY_SYSTEM_REBOOT   (1 << 7)
#define CAPABILITY_SYSTEM_SUSPEND  (1 << 8)

#define CAPSET_ALL ((CAPABILITY_SYSTEM_SUSPEND << 1) - 1)

#define CAPSET_HAS(capset, capability) (!!((capset) & (capability)))

/**
 * A set of available or restricted capabilities.
 * Each capability's index also represents its associated command identifier.
 */
typedef uint32_t capset_t;

/* CAPABILITIES_H */
#endif
