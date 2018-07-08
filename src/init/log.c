#include "log.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

static FILE *out;

/* lazy initialization of logs */
static void
log_lazy(void) {

	if(out == NULL) {
		out = stdout;
	}
}

void
log_print(const char *format,
	...) {
	va_list ap;
	log_lazy();

	va_start(ap, format);
	vfprintf(out, format, ap);
	va_end(ap);
}

void
log_error(const char *message) {
	log_lazy();

	fprintf(out, "%s: %s\n", message, strerror(errno));
}

