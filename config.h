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

/* CONFIG_H */
#endif
