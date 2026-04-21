#pragma once
#include <stdatomic.h>

typedef atomic_flag Spinlock;

#define spinlock_init(lock) \
    atomic_flag_clear(lock)

#define spinlock_lock(lock) \
    while (atomic_flag_test_and_set(lock)) { /* spin */ }

#define spinlock_unlock(lock) \
    atomic_flag_clear(lock)
