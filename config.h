#ifndef CONFIG_H
#define CONFIG_H

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

static const char *configurationdirs[] UNUSED = {
	"./test/daemons"
};

#define CONFIG_CONNECTIONS_LIMIT 64

#define CONFIG_CONTROLLERS_DIRECTORY "./test"
#define CONFIG_INITCTL_PATH CONFIG_CONTROLLERS_DIRECTORY"/initctl"

#define CONFIG_READ_BUFFER_SIZE 512

#define CONFIG_STDOUT_LOG 1
#define CONFIG_FULL_CLEANUP 1

/* CONFIG_H */
#endif
