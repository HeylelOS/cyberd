/* SPDX-License-Identifier: BSD-3-Clause */
#include "log.h"

#include <stdarg.h> /* va_list, va_start, va_end */
#include <syslog.h> /* setlogmask, openlog, closelog, vsyslog */
#include <libgen.h> /* basename */

#ifndef NDEBUG
#include <stdio.h> /* vfprintf, fputc */
#endif

#define LOG_OPTIONS (LOG_PID | LOG_CONS | LOG_NDELAY | LOG_NOWAIT)

/**
 * Opens log using @p progname as an identifier.
 * @param progname Program name used to identify logs.
 */
void
log_setup(char *progname) {
	setlogmask(LOG_MASK(LOG_INFO) | LOG_MASK(LOG_ERR));
	openlog(basename(progname), LOG_OPTIONS, LOG_USER);
}

/** Closes log facility. */
void
log_teardown(void) {
	closelog();
}

/** Log info. */
void
log_info(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
#ifdef NDEBUG
	vsyslog(LOG_INFO, format, ap);
#else
	vfprintf(stderr, format, ap);
	fputc('\n', stderr);
#endif
	va_end(ap);
}

/** Log error. */
void
log_error(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_ERR, format, ap);
	va_end(ap);
}
