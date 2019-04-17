#ifndef HASH_H
#define HASH_H

#include <stdint.h>     /* uint64_t */

/** A really stupid hash, but sufficient for now */
typedef uint64_t hash_t;

/**
 * Hashes a string, to avoid calling hash_start() and hash_update()
 * @param string NUL-terminated string to hash
 * @return hash
 */
hash_t
hash_string(const char *string);

/**
 * Starts a hash, for further updates
 * @return Empty value of a hash
 */
hash_t
hash_start(void);

/**
 * Updates a hash by a value
 * @param hash Hash to increase
 * @param c Byte by which we update hash
 * @return New hash
 */
hash_t
hash_update(hash_t hash, char c);

/* HASH_H */
#endif
