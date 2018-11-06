#ifndef LOG_H
#define LOG_H

void
log_init(void);

void
log_print(const char *format,
	...);

void
log_error(const char *message);

/* LOG_H */
#endif
