#ifndef CYBERCTL_H
#define CYBERCTL_H

int
cyberctl_connect(const char *path);

void
cyberctl_disconnect(int fd);

/* CYBERCTL_H */
#endif
