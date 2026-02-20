// TODO: Refactor this out? Idk how necessary it is. I feel like this is just way simpler.
// Then again a preboot will just fix all of this anyway so I'm postponing :)
#include <limine.h>
#include "log.h"
#include "printk.h"
#include "kpanic.h"

static volatile struct limine_smp_request limine_smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .flags = 0,
};
extern void ap_init(struct limine_smp_info*);
#define AP_STACK_SIZE 1*PAGE_SIZE
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/enable_arch_extra.h"
#include "apic.h"

static Mutex tss_sync = { 0 };
void ap_main(struct limine_smp_info* info) {
    reload_idt();
    reload_gdt();
    kernel_reload_gdt_registers();
    mutex_lock(&tss_sync);
    reload_tss();
    mutex_unlock(&tss_sync);
    __asm__ volatile(
            "movq %0, %%cr3\n"
            :
            : "r" ((uintptr_t)kernel.pml4 & ~KERNEL_MEMORY_MASK)
        );
    // APIC divider of 16
    printk("[CORE] Hello from logical processor %zu lapic_id %zu\n", info->lapic_id, get_lapic_id());
    enable_cpu_features();
    kernel.processors[info->lapic_id].initialised = true;
    lapic_timer_reload();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    asm volatile( "int $0x20" );
}

#define MAX_CPUS 16
static uint8_t kernel_ap_stacks[MAX_CPUS][AP_STACK_SIZE] __attribute__((aligned(PAGE_SIZE)));

void init_smp(void) {
    if(!limine_smp_request.response) return;
    size_t cpu_count = limine_smp_request.response->cpu_count;
    // TODO: Allow unlimited CPU cores
    if(cpu_count > MAX_CPUS) {
        kpanic("Too many CPU cores! Maximix cores allowed = %u, found %zu", MAX_CPUS, cpu_count);
    }
    // Mark BSP as initialised
    kernel.processors[limine_smp_request.response->bsp_lapic_id].initialised = true;
    for(size_t i = 0; i < cpu_count; ++i) {
        struct limine_smp_info* info = limine_smp_request.response->cpus[i];
        if(info->lapic_id == limine_smp_request.response->bsp_lapic_id)
            continue;
        if(kernel.max_processor_id < info->lapic_id)
            kernel.max_processor_id = info->lapic_id;
        // Assign stack from preallocated array instead of malloc
        info->extra_argument = (uintptr_t)(kernel_ap_stacks[i] + AP_STACK_SIZE);
        info->goto_address = (void*)&ap_init;
    }
}
