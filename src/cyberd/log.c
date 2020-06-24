#include "log.h"

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <syslog.h>

#define LOG_OPTIONS (LOG_PID | LOG_CONS | LOG_NDELAY | LOG_NOWAIT)

#ifdef NDEBUG
#include <libgen.h>

static const char *ident;
#endif

void
log_init(char *cyberdname) {
#ifdef NDEBUG
	ident = basename(cyberdname);

	setlogmask(LOG_MASK(LOG_INFO) | LOG_MASK(LOG_ERR));
	openlog(ident, LOG_OPTIONS, LOG_USER);
#endif
}

void
log_deinit(void) {

#ifdef NDEBUG
	closelog();
#endif
}

void
log_restart(void) {

#ifdef NDEBUG
	closelog();
	openlog(ident, LOG_OPTIONS, LOG_USER);
#endif
}

void
log_print(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
#ifdef NDEBUG
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
#ifdef NDEBUG
	vsyslog(LOG_ERR, format, ap);
#else
	vfprintf(stderr, format, ap);
	putc('\n', stderr);
#endif
	va_end(ap);
}

