#ifndef HASH_H
#define HASH_H

#include <stdint.h>     /* uint64_t */

typedef uint64_t hash_t;

hash_t
hash_string(const char *string);

/* HASH_H */
#endif
