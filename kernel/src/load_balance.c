#include "load_balance.h"
#include "kernel.h"
#include <sync/mutex.h>

size_t pick_processor_for_task(void) {
    size_t total = kernel.max_processor_id + 1;

    mutex_lock(&kernel.load_balancer_lock);
    size_t head = kernel.load_balancer_head;
    kernel.load_balancer_head = head + 1;
    mutex_unlock(&kernel.load_balancer_lock);

    size_t start = head % total;

    for (size_t i = 0; i < total; i++) {
        size_t id = (start + i) % total;
        if (kernel.processors[id].initialised)
            return id;
    }

    return 0; // doesn't happen unless system is cursed
}
