#include "hash.h"

#include <string.h>

/* Should implement a better one, FNV hash will do temporarily */

#define FNV_OFFSET_BASIS 0xCBF29CE484222325	
#define FNV_PRIME        0x100000001B3

hash_t
hash_string(const char *string) {
	const uint8_t *ptr = (const uint8_t *) string;
	const uint8_t * const end = ptr + strlen(string);
	hash_t hash = FNV_OFFSET_BASIS;

	while (ptr != end) {
		hash *= FNV_PRIME;
		hash ^= *ptr;

		ptr++;
	}

	return hash;
}

hash_t
hash_start(void) {
	return FNV_OFFSET_BASIS;
}

hash_t
hash_update(hash_t hash, char c) {
	hash *= FNV_PRIME;
	hash ^= c;

	return hash;
}

