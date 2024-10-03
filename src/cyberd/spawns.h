/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SPAWNS_H
#define SPAWNS_H

#include <sys/types.h> /* pid_t */

struct daemon;

void
spawns_record(struct daemon *daemon);

struct daemon *
spawns_retrieve(pid_t pid);

bool
spawns_empty(void);

void
spawns_stop(void);

#ifndef NDEBUG
void
spawns_cleanup(void);
#endif

/* SPAWNS_H */
#endif
