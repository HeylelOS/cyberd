#ifndef CONFIG_H
#define CONFIG_H

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

static const char *configurationdirs[] UNUSED = {
	"./test/daemons"
};

#define CONFIG_DEFAULT_UMASK (S_IWGRP | S_IWOTH)

#define CONFIG_CONNECTIONS_LIMIT 64
#define CONFIG_MAX_ACCEPTOR_LEN 255

#define CONFIG_CONTROLLERS_DIRECTORY "./test"
#define CONFIG_INITCTL_PATH CONFIG_CONTROLLERS_DIRECTORY"/initctl"

#define CONFIG_READ_BUFFER_SIZE 512

#define CONFIG_DEBUG 1

#ifdef CONFIG_DEBUG
#define CONFIG_STDOUT_LOG 1
#define CONFIG_FULL_CLEANUP 1
#endif

/* CONFIG_H */
#endif
