#ifndef CONFIG_H
#define CONFIG_H

/* On Darwin, it seems like htonll and ntohll are imported with libc */
#ifndef __APPLE__

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x) (x)
#define htonll(x) (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define bswap_64(x)\
	((((x) & 0xff00000000000000ull) >> 56)\
	| (((x) & 0x00ff000000000000ull) >> 40)\
	| (((x) & 0x0000ff0000000000ull) >> 24)\
	| (((x) & 0x000000ff00000000ull) >> 8)\
	| (((x) & 0x00000000ff000000ull) << 8)\
	| (((x) & 0x0000000000ff0000ull) << 24)\
	| (((x) & 0x000000000000ff00ull) << 40)\
	| (((x) & 0x00000000000000ffull) << 56))
#define ntohll(x) bswap_64(x)
#define htonll(x) bswap_64(x)
#else
#error "Unsupported machine word endianness"
#endif

#endif

#ifdef NDEBUG
/* Release */

#define CONFIG_DAEMONCONFS_DIRECTORY "/etc/daemons"
#define CONFIG_ENDPOINTS_DIRECTORY "/run/init"

#else
/* Debug */

#define CONFIG_DAEMONCONFS_DIRECTORY "daemons"
#define CONFIG_ENDPOINTS_DIRECTORY "init"

#define CONFIG_FULL_CLEANUP
#endif

#define CONFIG_ENDPOINT_ROOT "initctl"

#define CONFIG_DEFAULT_UMASK 022

#define CONFIG_CONNECTIONS_LIMIT 64

#define CONFIG_READ_BUFFER_SIZE 512

#define CONFIG_NAME_BUFFER_DEFAULT_CAPACITY 16

/* CONFIG_H */
#endif
