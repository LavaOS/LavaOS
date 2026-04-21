#pragma once
#include "delay.h"
#include "printk.h"
#include "cmdline.h"
#include "kernel.h"
#include "mc.h"
#include "term/fb/fb.h"

// This void will deinit all drivers before shutdown/reboot
// TODO: More deinit's

void do_poweroff_tasks() {
    uint64_t kernel_cache_cache = measure_cache_bytes(kernel.cache_cache);
    uint64_t kernel_inode_cache = measure_cache_bytes(kernel.inode_cache);
    uint64_t kernel_task_cache = measure_cache_bytes(kernel.task_cache);
    uint64_t kernel_resource_cache = measure_cache_bytes(kernel.resource_cache);
    uint64_t kernel_memregion_cache = measure_cache_bytes(kernel.memregion_cache);
    uint64_t kernel_memlist_cache = measure_cache_bytes(kernel.memlist_cache);
    uint64_t kernel_process_cache = measure_cache_bytes(kernel.process_cache);
    uint64_t kernel_allocation_cache = measure_cache_bytes(kernel.allocation_cache);
    uint64_t kernel_heap_cache = measure_cache_bytes(kernel.heap_cache);
    uint64_t kernel_charqueue_cache = measure_cache_bytes(kernel.charqueue_cache);
    uint64_t kernel_pci_device_cache = measure_cache_bytes(kernel.pci_device_cache);
    uint64_t kernel_cache64_cache = measure_cache_bytes(kernel.cache64);
    uint64_t kernel_cache256_cache = measure_cache_bytes(kernel.cache256);
    uint64_t kernel_shared_memory_cache = measure_cache_bytes(kernel.shared_memory_cache);

    kclear(0x000000);

    deinit_cmdline();
    deinit_fbtty();

    printk("All drivers deinited!\n");
    printk("DEBUG: cache size: %lu\n", (kernel_cache_cache + kernel_inode_cache + kernel_task_cache + kernel_resource_cache + kernel_memregion_cache + kernel_memlist_cache + kernel_process_cache + kernel_allocation_cache + kernel_heap_cache + kernel_charqueue_cache + kernel_pci_device_cache + kernel_cache64_cache + kernel_cache256_cache + kernel_shared_memory_cache));

    cache_dealloc(kernel.cache_cache, do_poweroff_tasks);
    cache_dealloc(kernel.inode_cache, do_poweroff_tasks);
    cache_dealloc(kernel.task_cache, do_poweroff_tasks);
    cache_dealloc(kernel.resource_cache, do_poweroff_tasks);
    cache_dealloc(kernel.memregion_cache, do_poweroff_tasks);
    cache_dealloc(kernel.memlist_cache, do_poweroff_tasks);
    cache_dealloc(kernel.process_cache, do_poweroff_tasks);
    cache_dealloc(kernel.allocation_cache, do_poweroff_tasks);
    cache_dealloc(kernel.heap_cache, do_poweroff_tasks);
    cache_dealloc(kernel.charqueue_cache, do_poweroff_tasks);
    cache_dealloc(kernel.pci_device_cache, do_poweroff_tasks);
    cache_dealloc(kernel.cache64, do_poweroff_tasks);
    cache_dealloc(kernel.cache256, do_poweroff_tasks);
    cache_dealloc(kernel.shared_memory_cache, do_poweroff_tasks);

    printk("Caches Deallocated!");

    delay(3);
}
