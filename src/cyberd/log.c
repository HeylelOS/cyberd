#include "log.h"

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <syslog.h>
#include <libgen.h>

#define LOG_OPTIONS (LOG_PID | LOG_CONS | LOG_NDELAY | LOG_NOWAIT)

static const char *ident;

void
log_init(char *cyberdname) {
	ident = basename(cyberdname);

	setlogmask(LOG_MASK(LOG_INFO) | LOG_MASK(LOG_ERR));
	openlog(ident, LOG_OPTIONS, LOG_USER);
}

void
log_deinit(void) {

	closelog();
}

void
log_restart(void) {

	closelog();
	openlog(ident, LOG_OPTIONS, LOG_USER);
}

void
log_print(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_INFO, format, ap);
	va_end(ap);
}

void
log_error(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_ERR, format, ap);
	va_end(ap);
}

