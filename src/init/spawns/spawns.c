#include "spawns.h"

#include <stdlib.h>

struct spawn {
	struct daemon *daemon;
	struct spawn *next;
};

static struct spawn *
spawn_create(struct daemon *daemon,
	pid_t pid,
	struct spawn *next) {
	struct spawn *spawn = malloc(sizeof(*spawn));

	daemon->pid = pid;
	spawn->daemon = daemon;
	spawn->next = next;

	return spawn;
}

static void
spawn_destroy(struct spawn *spawn) {

	free(spawn);
}

void
spawns_init(struct spawns *spawns) {
	spawns->first = NULL;
}

void
spawns_destroy(struct spawns *spawns) {
	struct spawn *current = spawns->first;

	while(current != NULL) {
		struct spawn *next = current->next;

		spawn_destroy(current);

		current = next;
	}
}

void
spawns_record(struct spawns *spawns,
	struct daemon *daemon,
	pid_t pid) {
	struct spawn *spawn = spawn_create(daemon, pid, spawns->first);

	spawns->first = spawn;
}

struct daemon *
spawns_retrieve(struct spawns *spawns,
	pid_t pid) {
	struct spawn **current = &spawns->first;
	struct daemon *daemon = NULL;

	while(*current != NULL
		&& (*current)->daemon->pid != pid) {

		current = &(*current)->next;
	}

	if(*current != NULL) {
		struct spawn *spawn = *current;

		daemon = spawn->daemon;
		daemon->pid = -1;
		*current = spawn->next;
		spawn_destroy(spawn);
	}

	return daemon;
}

