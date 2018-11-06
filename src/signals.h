#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void
signals_init(void);

extern const sigset_t signals_selmask; /* Authorized signals while in pselect */

/* SIGNALS_H */
#endif
