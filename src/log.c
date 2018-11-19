#include "log.h"

#include <syslog.h>
#include <stdarg.h>

void
log_init(void) {

	setlogmask(LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_ERR));
	openlog("cyberd", LOG_NDELAY | LOG_PID, LOG_USER);
}

void
log_deinit(void) {

	closelog();
}

void
log_print(const char *format,
	...) {
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_NOTICE, format, ap);
	va_end(ap);
}

void
log_error(const char *message) {

	syslog(LOG_ERR, "%s: %m\n", message);
}

