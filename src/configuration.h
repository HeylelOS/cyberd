#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/**
 * Initializes data structures, load daemons
 * for the first time, may start a daemon, that's
 * why it must be called after all other general *_init
 */
void
configuration_init(void);

/**
 * Re-read daemons, add new ones, reconfigure previous
 * ones, NOT restarting a daemon if it was launched,
 * dereferences and frees removed ones */
void
configuration_reload(void);

/* CONFIGURATION_H */
#endif
