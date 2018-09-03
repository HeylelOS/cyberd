#ifndef CYBERCTL_H
#define CYBERCTL_H

typedef struct cyberctl cyberctl_t;

enum cyberctl_status {
	RUNNING,
	STOPPED,
	STOPPING
};

struct cyberctl_state {
	enum cyberctl_status status;
	const char name[];
};

cyberctl_t *
cyberctl_create(void);

int
cyberctl_list(cyberctl_t *ctl,
	struct cyberctl_state *statuses,
	unsigned long *count);

void
cyberctl_destroy(cyberctl_t *ctl);

/* CYBERCTL_H */
#endif
