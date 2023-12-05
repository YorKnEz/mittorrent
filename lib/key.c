#include "key.h"

void key_from_addr(key2_t *key, struct sockaddr_in *addr) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (BYTE*)addr, sizeof(struct sockaddr_in));
    sha256_final(&ctx, key->key);
}

int32_t key_from_file(key2_t *key, int32_t fd, uint64_t *size) {
    // get the id of the file by hashing it
    SHA256_CTX ctx;
    sha256_init(&ctx);

    uint64_t read_bytes;
    char buf[BLOCK_SIZE];
    if (size) {
        *size = 0;
    }

    while ((read_bytes = read(fd, buf, BLOCK_SIZE))) {
        if (-1 == read_bytes) {
            print(LOG_ERROR, "[create_file] Error at read");
            return -1;
        }

        sha256_update(&ctx, buf, read_bytes);
		
        if (size) {
            *size += read_bytes;
        }
    }

    sha256_final(&ctx, key->key);

    return 0;
}

int32_t key_cmp(key2_t *key1, key2_t *key2) {
	for (int32_t i = 0; i < SHA256_BLOCK_SIZE; i++) {
		if (key1->key[i] > key2->key[i]) return 1;
		if (key1->key[i] < key2->key[i]) return -1;
	}
	return 0;
}

int32_t key_in(key2_t *key, key2_t *min, int32_t min_eq, key2_t *max, int32_t max_eq) {
	int32_t res1 = key_cmp(min, max),
		res2 = key_cmp(min, key),
		res3 = key_cmp(key, max);

	if (min_eq == 0 && max_eq == 0) {
		return (res1 < 0 && (res2 < 0 && res3 < 0)) || (res1 >= 0 && (res2 < 0 || res3 < 0));
	} else if (min_eq == 0 && max_eq == 1) {
		return (res1 < 0 && (res2 < 0 && res3 <= 0)) || (res1 >= 0 && (res2 < 0 || res3 <= 0));
		
	} else if (min_eq == 1 && max_eq == 0) {
		return (res1 < 0 && (res2 <= 0 && res3 < 0)) || (res1 >= 0 && (res2 <= 0 || res3 < 0));

	} else if (min_eq == 1 && max_eq == 1) {
		return (res1 < 0 && (res2 <= 0 && res3 <= 0)) || (res1 >= 0 && (res2 <= 0 || res3 <= 0));
	}
}

void key_add(key2_t *key1, key2_t *key2) {
	for (int32_t i = SHA256_BLOCK_SIZE - 1; i >= 0; i--) {
		uint16_t total;
		total = (uint16_t)key1->key[i] + (uint16_t)key2->key[i];
		key1->key[i] = total & 0xFF;

		// move carry
		if (i > 0) {
			key1->key[i - 1] += (total >> 8) & 0xFF;
		}
	}
}

void key_add_int(key2_t *key, uint32_t x){
	for (int32_t i = 3; i >= 0; i--) {
		uint16_t total;
		total = (uint16_t)key->key[i] + (uint16_t)((x >> (3 - i)) && 0xFF);
		key->key[i] = total & 0xFF;

		// move carry
		if (i > 0) {
			key->key[i - 1] += (total >> 8) & 0xFF;
		}
	}
}

void key_sub(key2_t *key1, key2_t *key2) {
	for (int32_t i = SHA256_BLOCK_SIZE - 1; i >= 0; i--) {
		uint16_t total;
		total = (uint16_t)key1->key[i] - (uint16_t)key2->key[i];
		key1->key[i] = total & 0xFF;

		// move carry
		if (i > 0) {
			key1->key[i - 1] += (total >> 8) & 0xFF;
		}
	}
}

void key_sub_int(key2_t *key, uint32_t x) {
	for (int32_t i = 3; i >= 0; i--) {
		uint16_t total;
		total = (uint16_t)key->key[i] - (uint16_t)((x >> (3 - i)) && 0xFF);
		key->key[i] = total & 0xFF;

		// move carry
		if (i > 0) {
			key->key[i - 1] += (total >> 8) & 0xFF;
		}
	}
}

void key_double(key2_t *key) {
	for (int32_t i = 0; i < SHA256_BLOCK_SIZE; i++) {
		BYTE carry = i == SHA256_BLOCK_SIZE - 1 ? 0 : (key->key[i - 1] >> 7);
		key->key[i] = (key->key[i] << 1) + carry;
	}
}

void print_key(key2_t *key) {
	// for (int32_t i = 0; i < 32; i++) {
	// 	print(LOG_DEBUG, "%x", key->key[i]);
	// }
	// temp alternative for shorter prints
	print(LOG_DEBUG, "%x%x..%x%x", key->key[0], key->key[1], key->key[30], key->key[31]);
}