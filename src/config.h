#ifndef CONFIG_H
#define CONFIG_H

#ifndef CYBERD_UNUSED
#define CYBERD_UNUSED __attribute__((unused))
#endif

static const char *configurationdirs[] CYBERD_UNUSED = {
	"./test/daemons"
};

#define DISPATCHER_IPC_SOCKNAME "./test/ipc"
#define DISPATCHER_CONNECTIONS_LIMIT	64

/* CONFIG_H */
#endif
