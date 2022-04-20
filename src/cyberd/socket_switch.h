/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SOCKET_SWITCH_H
#define SOCKET_SWITCH_H

#include <sys/select.h> /* fd_set */

struct socket_node;

void
socket_switch_setup(void);

void
socket_switch_teardown(void);

void
socket_switch_insert(struct socket_node *snode);

void
socket_switch_remove(struct socket_node *snode);

int
socket_switch_prepare(fd_set **readfdsp, fd_set **writefdsp, fd_set **exceptfdsp);

void
socket_switch_operate(int nfds);

/* SOCKET_SWITCH_H */
#endif
