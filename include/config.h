#ifndef CONFIG_H
#define CONFIG_H

#ifdef NDEBUG
/* Release */

#define CONFIG_DAEMONCONFS_DIRECTORY "/etc/daemons"
#define CONFIG_CONTROLLERS_DIRECTORY "/run/init"

#else
/* Debug */

#define CONFIG_DAEMONCONFS_DIRECTORY "daemons"
#define CONFIG_CONTROLLERS_DIRECTORY "init"

#define CONFIG_FULL_CLEANUP
#endif

#define CONFIG_CONTROLLERS_FIRST "initctl"

#define CONFIG_DEFAULT_UMASK 022

#define CONFIG_CONNECTIONS_LIMIT 64

#define CONFIG_READ_BUFFER_SIZE 512

#define CONFIG_NAME_BUFFER_DEFAULT_CAPACITY 16

/* CONFIG_H */
#endif
