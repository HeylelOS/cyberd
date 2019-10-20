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

#ifndef CONFIG_DEBUG
	setlogmask(LOG_MASK(LOG_INFO) | LOG_MASK(LOG_ERR));
	openlog(ident, LOG_OPTIONS, LOG_USER);
#endif
}

void
log_deinit(void) {

#ifndef CONFIG_DEBUG
	closelog();
#endif
}

void
log_restart(void) {

#ifndef CONFIG_DEBUG
	closelog();
	openlog(ident, LOG_OPTIONS, LOG_USER);
#endif
}

void
log_print(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
#ifndef CONFIG_DEBUG
	vsyslog(LOG_INFO, format, ap);
#else
	vprintf(format, ap);
	putchar('\n');
#endif
	va_end(ap);
}

void
log_error(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
#ifndef CONFIG_DEBUG
	vsyslog(LOG_ERR, format, ap);
#else
	vfprintf(stderr, format, ap);
	putc('\n', stderr);
#endif
	va_end(ap);
}

