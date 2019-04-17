#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

extern const sigset_t signals_selmask; /* Authorized signals while in pselect */

/**
 * Initializes signal mask, handlers etc...
 */
void
signals_init(void);

/**
 * Sets all signals as needed for when we stop all daemons.
 */
void
signals_stopping(void);

/**
 * Sets all signals as needed for when we terminate definitely
 */
void
signals_ending(void);

/* SIGNALS_H */
#endif
