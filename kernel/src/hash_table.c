#include "hash_table.h"
#include "memory.h"
#include "printk.h"
#include <stdlib.h>

void ht_init(hash_table* ht) {
    for(u32 i = 0; i < HT_SIZE; i++)
        ht->buckets[i] = NULL;
}
void ht_insert(hash_table* ht, void* key, void* value) {
    u32 h = hash_ptr(key, HT_BITS);
    ht_entry* e = (ht_entry*)kernel_malloc(sizeof(ht_entry));
    e->key = key;
    e->value = value;
    e->next = ht->buckets[h];
    ht->buckets[h] = e;
}
void* ht_lookup(hash_table* ht, void* key) {
    u32 h = hash_ptr(key, HT_BITS); ht_entry* e = ht->buckets[h];
    while(e) {
        if(e->key == key) return e->value;
        e = e->next;
    }
    return NULL;
}
void ht_delete(hash_table* ht, void* key) {
    u32 h = hash_ptr(key, HT_BITS);
    ht_entry* e = ht->buckets[h];
    ht_entry* prev = NULL;
    while (e) {
        if (e->key == key) {
            if (prev)
                prev->next = e->next;
            else
                ht->buckets[h] = e->next;
            kernel_dealloc(e, sizeof(ht_entry));
            return;
        }
        prev = e;
        e = e->next;
    }
    printk("[HASH] Key %p not found for delete\n", key);
}
void ht_overwrite(hash_table* ht, void* key, void* new_value) {
    u32 h = hash_ptr(key, HT_BITS);
    ht_entry* e = ht->buckets[h];
    while (e) {
        if (e->key == key) {
            e->value = new_value;
            return;
        }
        e = e->next;
    }
    printk("[HASH] Key %p not found for overwrite\n", key);
}
void ht_dump(hash_table* ht) {
    printk("[HASH] Dumping hash table:\n");
    for (u32 i = 0; i < HT_SIZE; i++) {
        ht_entry* e = ht->buckets[i];
        if (!e) continue;
        printk("  bucket[%u]:\n", i);
        while (e) {
            printk("    key=%p value=%s\n", e->key,
                   e->value ? (char*)e->value : "NULL");
            e = e->next;
        }
    }
    printk("[HASH] Dump finished.\n");
}
