#include "../../config.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "port.h"
#include "serial.h"
#include "logger.h"
#include "log.h"
#include "assert.h"
#include "print.h"
#include "print_base.h"
#include "utils.h"
#include "memory.h"
#include "mem/bitmap.h"
#include "kernel.h"
#include "page.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/enable_arch_extra.h"
#include "arch/x86_64/exception.h"
#include "vfs.h"
#include "rootfs.h"
#include "mem/slab.h"
#include "string.h"
#include "process.h"
#include "task.h"
#include "task_switch.h"
#include "exec.h"
#include "pic.h"
#include "devices.h"
#include "./devices/tty/tty.h"
#include <minos/keycodes.h>
#include <minos/key.h>
#include <sync.h>
#include "cmdline.h"
#include "charqueue.h"
#include "filelog.h"
#include "iomem.h"
#include "acpi.h"
#include "apic.h"
#include "pci.h"
#include "interrupt.h"
#include "general_caches.h"
#include "epoll.h"
#include "sockets/minos.h"
#include "smp.h"
#include "mem/shared_mem.h"
#include "printk.h"
#include "term/fb/fb.h"
#include "ide/ide.h"

#include "delay.h"
#include "rtc.h"

void spawn_init(void) {
    intptr_t e = 0;
    const char* epath = NULL;
    Args args;
    Args env;
    epath = "/sbin/init";
    const char* argv[] = {epath, "test_arg", NULL};
    args = create_args(argv);
    const char* envv[] = {"FOO=BAR", "BAZ=A", NULL};
    env  = create_args(envv);
    if((e = exec_new(epath, &args, &env)) < 0) kpanic("Failed to exec %s : %s",epath,status_str(e));
    kinfo("Spawning `%s` id=%zu tid=%zu", epath, (size_t)e, ((Process*)(kernel.processes.items[(size_t)e]))->main_thread->id);
}
void _start() {
    disable_interrupts();
    BREAKPOINT();
    serial_init();
    kernel.logger = &serial_logger;
    kernel.logger->level = LOG_ALL;
    init_cmdline();
    init_loggers();
    init_gdt();
    disable_interrupts();
    init_idt();
    init_exceptions();
    reload_tss();
    init_bitmap();
    init_paging();
    KERNEL_SWITCH_VTABLE();
    enable_cpu_features();
    // Interrupt controller Initialisation
    printk("Starting Interrupt controller...\n");
    init_pic();
    init_acpi();
    printk("Started Interrupt controller.\n");
    enable_interrupts();
    // Caches
    printk("Configuring caches...\n");
    init_cache_cache();
    minos_socket_init_cache();
    init_epoll_cache();
    init_general_caches();
    init_charqueue();
    printk("Caches are ok.\n");
    // Devices
    printk("Initilazing devices...\n");
    printk("Loading PCI...\n");
    init_pci();
    // SMP
    printk("Loading SMP...\n");
    init_smp();
    printk("Devices loaded.\n");
    // Initialisation for process related things
    printk("Configuring memory...\n");
    init_memregion();
    init_processes();
    init_tasks();
    init_kernel_task();
    init_schedulers();
    init_task_switch();
    init_resources();
    init_shm_cache();

    disable_interrupts();
    calibrate_tsc();
    printk("CPU calibrated at ~%llu Hz\n", tsc_hz);

    delay(1);
    printk("1 second passed!\n");

    rtc_init();
    rtc_print_time();

    printk("Starting IDE...\n");
    ide_init();
    delay(1);

    // VFS
    enable_interrupts();
    printk("Initilazing filesystms...\n");
    init_vfs();
    init_rootfs();
    init_devices();
    init_tty();

    printk("Spawning init...\n");

    spawn_init();

    disable_interrupts();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    for(;;) {
        asm volatile("hlt");
    }
}
