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
            kernel_free(e, sizeof(ht_entry));
            printk("[HASH] Deleted key %p\n", key);
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
            printk("[HASH] Overwritten key %p with new value %s\n", key,
                   new_value ? (char*)new_value : "NULL");
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
void ht_test() {
    printk("[HASH] Starting hash table test...\n");

    hash_table ht;
    ht_init(&ht);

    int k1 = 1, k2 = 2, k3 = 3;

    char* v1 = "one";
    char* v2 = "two";
    char* v3 = "three";

    // insert
    ht_insert(&ht, &k1, v1);
    ht_insert(&ht, &k2, v2);
    ht_insert(&ht, &k3, v3);

    printk("[HASH] Inserted 3 elements\n");

    // lookup
    printk("[HASH] k1 = %s\n", (char*)ht_lookup(&ht, &k1));
    printk("[HASH] k2 = %s\n", (char*)ht_lookup(&ht, &k2));
    printk("[HASH] k3 = %s\n", (char*)ht_lookup(&ht, &k3));

    // overwrite k3
    ht_overwrite(&ht, &k3, "THREE");
    printk("[HASH] After overwrite k3 = %s\n", (char*)ht_lookup(&ht, &k3));

    // delete k2
    ht_delete(&ht, &k2);

    // check after delete
    printk("[HASH] After delete k2 = %s\n",
           (char*)ht_lookup(&ht, &k2));

    ht_dump(&ht);

    printk("[HASH] Test finished.\n");
}
