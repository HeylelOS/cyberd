#ifndef FDE_H
#define FDE_H

#include "fde/perms.h"
#include "fde/control.h"

/** File Descriptor Element, for dispatcher */
struct fde {
	int fd;
	enum {
		FDE_TYPE_ACCEPTOR,
		FDE_TYPE_CONTROLLER
	} type;
	perms_t perms;
	struct control *control;
};

/**
 * Creates a new socket to listen to IPCs
 * @param path Path to the new socket in the filesystem
 * @param perms Allowed actions in the filesystem
 * @return Newly allocated File Descriptor Element, must be fde_destroy()'d
 */
struct fde *
fde_create_acceptor(const char *path, perms_t perms);

/**
 * Creates a new socket for incoming IPCs
 * @param acceptor Acceptor from which we create it and keep traces of permissions.
 * @return Newly allocated File Descriptor Element, must be fde_destroy()'d
 */
struct fde *
fde_create_controller(const struct fde *acceptor);

/**
 * Destroys a File Descriptor Element
 * @param fde A previously fde_create_acceptor()'d or fde_create_controller()'d fde
 */
void
fde_destroy(struct fde *fde);

/* FDE_H */
#endif
