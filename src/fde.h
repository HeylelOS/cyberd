#ifndef FDE_H
#define FDE_H

/* File Descriptor Element, for dispatcher */

enum fd_element_type {
	FD_TYPE_ACCEPTOR,
	FD_TYPE_CONNECTION
};

struct fd_element {
	enum fd_element_type type;
	int fd;
};

struct fd_element *
fde_create_acceptor(const char *path);

struct fd_element *
fde_create_connection(const struct fd_element *acceptor);

void
fde_destroy(struct fd_element *fde);

/* FDE_H */
#endif
