#ifndef KEY_H
#define KEY_H

#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include "sha256.h"

#define KEY_SIZE SHA256_BLOCK_SIZE
#define KEY_BITS 8 * SHA256_BLOCK_SIZE
#define BLOCK_SIZE 4096

// interface over sha256 so i can change the algorithm later
typedef struct {
    BYTE key[KEY_SIZE];
} key2_t;

// take a sockaddr_in struct and hash it
void key_from_addr(key2_t *key, struct sockaddr_in *addr);

// hash the contents of the file, also save the size of the file in size ptr
int32_t key_from_file(key2_t *key, int32_t fd, uint64_t *size);

// bitwise cmp
// key1 < key2 -> -1
// key1 = key2 ->  0
// key1 > key2 ->  1
int32_t key_cmp(key2_t *key1, key2_t *key2);

// checks if a value is in between min and max
// min_eq 	| max_eq 	| result
// 0		| 0			| key in (min, max)
// 0		| 1			| key in (min, max]
// 1		| 0			| key in [min, max)
// 1		| 1			| key in [min, max]
int32_t key_in(key2_t *key, key2_t *min, int32_t min_eq, key2_t *max, int32_t max_eq);

// perform key1 += key2
void key_add(key2_t *key1, key2_t *key2);

// performs hash1 += x
void key_add_int(key2_t *key, uint32_t x);

// performs key1 -= key2
void key_sub(key2_t *key1, key2_t *key2);

// performs key -= x
void key_sub_int(key2_t *key, uint32_t x);

// performs key *= 2
void key_double(key2_t *key);

void print_key(key2_t *key);

#endif