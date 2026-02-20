#pragma once
#include "hash.h"
#include <stdbool.h>
#include <stddef.h>

#define HT_BITS 8
#define HT_SIZE (1 << HT_BITS)

typedef struct ht_entry {
    void* key;
    void* value;
    struct ht_entry* next;
} ht_entry;

typedef struct hash_table {
    ht_entry* buckets[HT_SIZE];
} hash_table;

void ht_init(hash_table* ht);
void ht_insert(hash_table* ht, void* key, void* value);
void* ht_lookup(hash_table* ht, void* key);
void ht_delete(hash_table* ht, void* key);
void ht_overwrite(hash_table* ht, void* key, void* new_value);
void ht_dump(hash_table* ht);
void ht_test();
