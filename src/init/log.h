#ifndef LOG_H
#define LOG_H

void
init_log(void);

void
log_print(const char *format,
	...);

void
log_error(const char *message);

/* LOG_H */
#endif
