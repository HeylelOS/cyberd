#ifndef CYBERCTL_H
#define CYBERCTL_H

#include <sys/types.h>

typedef int cyberctl_t;

cyberctl_t
cyberctl_open(const char *path);

void
cyberctl_close(cyberctl_t ctl);

struct cyberctl_status {
	pid_t pid;
	const char name[];
};

int
cyberctl_status(cyberctl_t ctl,
	struct cyberctl_status *statuses,
	unsigned int *count);

/* CYBERCTL_H */
#endif
