#include "log.h"

#include "config.h"

#ifndef CONFIG_STDOUT_LOG
#include <syslog.h>
#include <libgen.h>
#else
#include <stdio.h>
#include <err.h>
#endif

#include <stdarg.h>

void
log_init(char *cyberdname) {

#ifndef CONFIG_STDOUT_LOG
	setlogmask(LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_ERR));
	openlog(basename(cyberdname), LOG_NDELAY | LOG_PID, LOG_USER);
#endif
}

void
log_deinit(void) {

#ifndef CONFIG_STDOUT_LOG
	closelog();
#endif
}

void
log_print(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
#ifndef CONFIG_STDOUT_LOG
	vsyslog(LOG_NOTICE, format, ap);
#else
	vprintf(format, ap);
#endif
	va_end(ap);
}

void
log_error(const char *message) {

#ifndef CONFIG_STDOUT_LOG
	syslog(LOG_ERR, "%s: %m\n", message);
#else
	warn("%s", message);
#endif
}
