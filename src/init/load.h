#ifndef LOAD_H
#define LOAD_H

#include "daemon.h"

#include <stdio.h>

int
configure(struct daemon *daemon,
	FILE *filep);

void
load(const char *directory);

/* LOAD_H */
#endif
