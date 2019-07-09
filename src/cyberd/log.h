#ifndef LOG_H
#define LOG_H

/** Init logging if needed */
void
log_init(char *cyberdname);

/** Closes logging if needed */
void
log_deinit(void);

/** Restarts logging */
void
log_restart(void);

/**
 * Prints in the log according to format
 * @param format Format to follow
 */
void
log_print(const char *format, ...);

/**
 * Prints the message followed by an error message depending on errno
 * @param message Message to prefix
 */
void
log_error(const char *format, ...);

/* LOG_H */
#endif
