#include "../log.h"
#include "networker.h"

#ifdef __APPLE__
/* FD_COPY uses bcopy in Darwin */
#include <strings.h>
#endif

void
networker_init(struct networker *networker) {

	FD_ZERO(&networker->activefds);
	networker->lastfd = 0;

}

void
networker_destroy(struct networker *networker) {
}

int
networker_lastfd(struct networker *networker) {
	return networker->lastfd;
}

fd_set *
networker_readset(struct networker *networker) {

	return NULL;
}

fd_set *
networker_writeset(struct networker *networker) {

	return NULL;
}

int
networker_reading(struct networker *networker,
	int fds) {

	return 0;
}

int
networker_writing(struct networker *networker,
	int fds) {

	return 0;
}

