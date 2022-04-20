/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h> /* sigset_t */

extern int signals_requested_reboot;

void
signals_setup(sigset_t *sigmaskp);

void
signals_stopping(void);

void
signals_ending(void);

/* SIGNALS_H */
#endif
