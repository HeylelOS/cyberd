#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

extern const sigset_t signals_selmask; /* Authorized signals while in pselect */

void
signals_init(void);

void
signals_stopping(void);

void
signals_ending(void);

/* SIGNALS_H */
#endif
