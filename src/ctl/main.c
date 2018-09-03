#include <cyberctl.h>

int
main(int argc,
	char **argv) {
	cyberctl_t *control = cyberctl_create();

	cyberctl_destroy(control);

	return 0;
}
