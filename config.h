#ifndef CONFIG_H
#define CONFIG_H

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#ifdef CONFIG_DEBUG
#define CONFIG_STDOUT_LOG 1
#define CONFIG_FULL_CLEANUP 1
#define CONFIG_CONTROLLERS_DIRECTORY "./test/cyberd"

static const char *configurationdirs[] UNUSED = {
	"./test/cyberd/daemons"
};
#else
#define CONFIG_CONTROLLERS_DIRECTORY "/var/init"

static const char *configurationdirs[] UNUSED = {
	"/etc/daemons"
};
#endif

#define CONFIG_INITCTL_PATH CONFIG_CONTROLLERS_DIRECTORY"/initctl"

#define CONFIG_DEFAULT_UMASK 022

#define CONFIG_CONNECTIONS_LIMIT 64
#define CONFIG_MAX_ACCEPTOR_LEN 255

#define CONFIG_READ_BUFFER_SIZE 512

/* CONFIG_H */
#endif
