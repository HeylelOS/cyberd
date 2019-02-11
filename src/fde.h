#ifndef FDE_H
#define FDE_H

#include "fde/perms.h"
#include "fde/control.h"

/* File Descriptor Element, for dispatcher */

struct fde {
	int fd;
	enum {
		FDE_TYPE_ACCEPTOR,
		FDE_TYPE_CONTROLLER
	} type;
	perms_t perms;
	struct control *control;
};

struct fde *
fde_create_acceptor(const char *path, perms_t perms);

struct fde *
fde_create_controller(const struct fde *acceptor);

void
fde_destroy(struct fde *fde);

/* FDE_H */
#endif
