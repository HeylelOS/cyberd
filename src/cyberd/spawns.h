/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SPAWNS_H
#define SPAWNS_H

#include "daemon.h"

#include <stdbool.h>

void
spawns_init(void);

void
spawns_stop(void);

bool
spawns_empty(void);

void
spawns_end(void);

void
spawns_record(struct daemon *daemon);

struct daemon *
spawns_retrieve(pid_t pid);

/* SPAWNS_H */
#endif
