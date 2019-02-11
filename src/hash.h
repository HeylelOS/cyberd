#ifndef HASH_H
#define HASH_H

#include <stdint.h>     /* uint64_t */

typedef uint64_t hash_t;

hash_t
hash_string(const char *string);

hash_t
hash_start(void);

hash_t
hash_run(hash_t hash, char c);

/* HASH_H */
#endif
