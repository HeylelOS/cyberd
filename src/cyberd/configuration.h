#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "daemon.h"
#include "config.h"

/**
 * Initializes data structures, load daemons
 * for the first time, may start a daemon, that's
 * why it must be called after all other general *_init
 */
void
configuration_init(void);

#ifdef CONFIG_FULL_CLEANUP
/**
 * Frees all internal structures
 */
void
configuration_deinit(void);
#endif

/**
 * Re-read daemons, add new ones, reconfigure previous
 * ones, NOT restarting a daemon if it was launched,
 * dereferences and frees removed ones
 */
void
configuration_reload(void);

/**
 * Fetches a daemon loaded from the configuration
 * beware a configuration reload may leave a free'd
 * pointer outside. At this point, only the scheduler
 * is filled with pointer fetched from this function.
 * and it is emptied at each reload.
 * @param name Name of the daemon
 * @return NULL if not found, the daemon else
 */
struct daemon *
configuration_daemon_find(const char *name);

/* CONFIGURATION_H */
#endif
