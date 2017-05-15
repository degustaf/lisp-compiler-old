#include "Murmur3.h"

static inline __attribute__((always_inline)) uint32_t rotl(uint32_t x, uint8_t r) {
	return (x << r) | (x >> (32-r));
}

static inline uint32_t fmix(uint32_t h) {
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

uint32_t hash32(const void *key, size_t len) {
	const uint8_t *data = (uint8_t*) key;
	const size_t nblocks = len / 4;
	uint32_t h1 = 0;
	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;
	const uint32_t c3 = 0xe6546b64;

	const uint32_t *block = (uint32_t*)data;
	for(size_t i = 0; i<nblocks; i++) {
		uint32_t k1 = *(block++);
		k1 *= c1;
		k1 = rotl(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = rotl(h1, 13);
		h1 = h1 *5 + c3;
	}

	const uint8_t *tail = data + 4*nblocks;
	uint32_t k1 = 0;
	switch(len & 3) {
		case 3: k1 ^= tail[2] << 16;
		case 2: k1 ^= tail[1] << 8;
		case 1: k1 ^= tail[0];

		k1 *= c1;
		k1 = rotl(k1, 15);
		k1 *= c2;
		h1 ^= k1;
	}

	h1 ^= (uint32_t)len;
	return fmix(h1);
}
