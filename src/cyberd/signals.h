/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h> /* sig_atomic_t, sigset_t */

extern volatile sig_atomic_t sigreboot, sigchld, sighup, sigalrm;

void
signals_setup(sigset_t *sigmaskp);

void
signals_alarm(sigset_t *sigmaskp, unsigned int seconds);

/* SIGNALS_H */
#endif
