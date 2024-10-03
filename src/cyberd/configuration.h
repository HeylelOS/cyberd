/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

struct daemon *
configuration_find(const char *name);

void
configuration_load(const char *path);

void
configuration_reload(void);

#ifndef NDEBUG
void
configuration_cleanup(void);
#endif

/* CONFIGURATION_H */
#endif
