#include "log.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

static FILE *out;

void
log_init(void) {
	out = stdout;
}

void
log_print(const char *format,
	...) {
	va_list ap;

	va_start(ap, format);
	vfprintf(out, format, ap);
	va_end(ap);
}

void
log_error(const char *message) {

	fprintf(out, "%s: %s\n", message, strerror(errno));
}

