/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LOG_H
#define LOG_H

void
log_setup(char *progname);

void
log_teardown(void);

void
log_info(const char *format, ...);

void
log_error(const char *format, ...);

/* LOG_H */
#endif
