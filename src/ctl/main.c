#include <cyberctl.h>
#include <stdlib.h>

int
main(int argc,
	char **argv) {
	int fd = cyberctl_connect(NULL);

	cyberctl_disconnect(fd);

	return 0;
}
