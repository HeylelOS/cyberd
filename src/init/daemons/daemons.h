#ifndef DAEMONS_H
#define DAEMONS_H

#include "../daemon.h"

/**
 * daemons.h holds the structure
 * which contains the lone allocation of each
 * daemon, its implementation is an AVL binary Tree
 */

struct daemons_node {
	struct daemon daemon;

	struct daemons_node *left, *right;
	int height;
};

struct daemons {
	struct daemons_node *root;
};

struct daemons_node *
daemons_node_create(const char *name);

void
daemons_node_destroy(struct daemons_node *node);

void
daemons_init(struct daemons *daemons);

void
daemons_destroy(struct daemons *daemons);

void
daemons_insert(struct daemons *daemons,
	struct daemons_node *node);

struct daemon *
daemons_remove(struct daemons *daemons,
	hash_t hash);

struct daemon *
daemons_find(struct daemons *daemons,
	hash_t hash);

/* DAEMONS_H */
#endif
