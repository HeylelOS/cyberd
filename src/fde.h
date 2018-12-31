#ifndef FDE_H
#define FDE_H

/* File Descriptor Element, for dispatcher */

typedef unsigned long fde_caps_t;
#define FDE_CAPS_ACCEPTOR		(1 << 0)
#define FDE_CAPS_ACCEPTOR_CREATOR	(1 << 1)

#define FDE_CAPS_DAEMONS_START	(1 << 8)
#define FDE_CAPS_DAEMONS_STOP	(1 << 9)
#define FDE_CAPS_DAEMONS_RELOAD	(1 << 10)
#define FDE_CAPS_DAEMONS_END	(1 << 11)
#define FDE_CAPS_DAEMONS_ALL	(FDE_CAPS_DAEMONS_START | FDE_CAPS_DAEMONS_STOP\
								| FDE_CAPS_DAEMONS_RELOAD | FDE_CAPS_DAEMONS_END)

#define FDE_CAPS_SHUTDOWN_HALT		(1 << 16)
#define FDE_CAPS_SHUTDOWN_REBOOT	(1 << 17)
#define FDE_CAPS_SHUTDOWN_SLEEP		(1 << 18)
#define FDE_CAPS_SHUTDOWN_ALL		(FDE_CAPS_SHUTDOWN_HALT | FDE_CAPS_SHUTDOWN_REBOOT\
									| FDE_CAPS_SHUTDOWN_SLEEP)

struct fd_element {
	fde_caps_t caps;
	int fd;
};

struct fd_element *
fde_create_acceptor(const char *path, fde_caps_t caps);

struct fd_element *
fde_create_connection(const struct fd_element *acceptor);

void
fde_destroy(struct fd_element *fde);

/* FDE_H */
#endif
