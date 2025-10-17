#include "memory.h"
#include "mem/bitmap.h"
#include "kernel.h"
paddr_t kernel_pages_alloc(size_t pages) {
    mutex_lock(&kernel.map_lock);
    void* addr = bitmap_alloc(&kernel.map, pages);
    paddr_t res = (paddr_t)((uintptr_t)addr & ~KERNEL_MEMORY_MASK);
    mutex_unlock(&kernel.map_lock);
    return res;
}
paddr_t kernel_page_alloc() {
    return kernel_pages_alloc(1);
}
void kernel_pages_dealloc(paddr_t page, size_t count) {
    mutex_lock(&kernel.map_lock);
    bitmap_dealloc(&kernel.map, (void*)page, count);
    mutex_unlock(&kernel.map_lock);
}
void kernel_page_dealloc(paddr_t page) {
    kernel_pages_dealloc(page, 1);
}
void* kernel_malloc(size_t size) {
    mutex_lock(&kernel.map_lock);
    size_t pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    void* addr = bitmap_alloc(&kernel.map, pages);
    if(addr) addr = (void*)((uintptr_t)addr | KERNEL_MEMORY_MASK);
    mutex_unlock(&kernel.map_lock);
    return addr;
}
void kernel_dealloc(void* data, size_t size) {
    if(!data) return;
    mutex_lock(&kernel.map_lock);
    size_t pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    bitmap_dealloc(&kernel.map, (void*)((uintptr_t)data & ~KERNEL_MEMORY_MASK), pages);
    mutex_unlock(&kernel.map_lock);
}
