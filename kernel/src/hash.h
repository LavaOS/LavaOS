// ported from linux 3.0
// https://github.com/torvalds/linux/blob/v3.0/include/linux/hash.h

/* Fast hashing routine for ints,  longs and pointers.
   (C) 2002 William Lee Irwin III, IBM */

#pragma once
#include "linux-asm/types.h"
// #include "linux-asm/bitsperlong.h"

#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

#if BITS_PER_LONG == 32
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_32
#define hash_long(val, bits) hash_32(val, bits)
#elif BITS_PER_LONG == 64
#define hash_long(val, bits) hash_64(val, bits)
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_64
#else
#error Wordsize not 32 or 64
#endif

static inline u64 hash_64(u64 val, unsigned int bits)
{
	u64 hash = val;
	u64 n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;
	return hash >> (64 - bits);
}

static inline u32 hash_32(u32 val, unsigned int bits)
{
	u32 hash = val * GOLDEN_RATIO_PRIME_32;
	return hash >> (32 - bits);
}

static inline unsigned long hash_ptr(void *ptr, unsigned int bits)
{
	return hash_long((unsigned long)ptr, bits);
}
