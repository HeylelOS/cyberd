#ifndef CONFIG_H
#define CONFIG_H

#ifndef CYBERD_UNUSED
#define CYBERD_UNUSED __attribute__((unused))
#endif

static const char *configurationdirs[] CYBERD_UNUSED = {
	"./test/daemons"
};

#define CYBERD_CONNECTIONS_LIMIT 64

#define CYBERCTL_IPC_PATH "./test/ipc"

/* CONFIG_H */
#endif
