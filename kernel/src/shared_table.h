#include "hash_table.h"
#include "kht.h"

void st_insert(void* key, void* value) {
    ht_insert(&kernel_ht, key, value);
}
void st_lookup(void* key) {
    ht_lookup(&kernel_ht, key);
}
void st_delete(void* key) {
    ht_delete(&kernel_ht, key);
}
void st_overwrite(void* key, void* new_value) {
    ht_overwrite(&kernel_ht, key, new_value);
}
