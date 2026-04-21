#pragma once
#include <mem/slab.h>
#include <stdint.h>
#include <stddef.h>

uint64_t measure_cache_bytes(const Cache* cache) {
    if (cache == NULL) {
        return 0;
    }
    uint64_t used_memory = (uint64_t)cache->inuse * (uint64_t)cache->objsize;
    return used_memory;
}
