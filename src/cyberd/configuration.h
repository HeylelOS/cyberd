/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "config.h"

struct daemon *
configuration_find(const char *name);

void
configuration_load(void);

void
configuration_reload(void);

#ifdef CONFIG_MEMORY_CLEANUP
void
configuration_teardown(void);
#endif

/* CONFIGURATION_H */
#endif
