#include "log.h"

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <syslog.h>
#include <libgen.h>

void
log_init(char *cyberdname) {

	setlogmask(LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_ERR));
	openlog(basename(cyberdname), LOG_PID | LOG_CONS | LOG_NDELAY | LOG_NOWAIT, LOG_USER);
}

void
log_deinit(void) {

	closelog();
}

void
log_print(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_NOTICE, format, ap);
	va_end(ap);
}

void
log_error(const char *format, ...) {
	size_t buffersize = CONFIG_LOG_ERROR_BUFFER_SIZE;
	va_list ap;

	va_start(ap, format);
	while(true) {
		char buffer[buffersize];

		if(vsnprintf(buffer, buffersize, format, ap) < buffersize) {
			syslog(LOG_ERR, "%s: %m\n", buffer);
			break;
		} else {
			buffersize *= 2;
		}
	}
	va_end(ap);
}

