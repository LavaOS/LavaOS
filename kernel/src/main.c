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

    printk("Welcome to LavaOS!\n\n");
    delay(3);

    printk("[WAIT] Initilazing serial...\n");
    serial_init();
    kernel.logger = &serial_logger;
    kernel.logger->level = LOG_ALL;
    printk("[ OK ] Initilazed serial.\n");
    printk("[WAIT] Initilazing cmdline...\n");
    init_cmdline();
    printk("[ OK ] Initilazed cmdline.\n");
    printk("[WAIT] Initilazing loggers...\n");
    init_loggers();
    printk("[ OK ] Initilazed loggers.\n");
    printk("[WAIT] Initilazing GDT and IDT...\n");
    init_gdt();
    disable_interrupts();
    init_idt();
    printk("[ OK ] Initilazed GDT and IDT.\n");
    printk("[WAIT] Initilazing essential components and devices...\n");
    init_exceptions();
    reload_tss();
    init_bitmap();
    init_paging();
    KERNEL_SWITCH_VTABLE();
    enable_cpu_features();
    printk("[ OK ] Initilazed essential components and devices.\n");
    printk("[WAIT]Starting Interrupt controller...\n");
    init_pic();
    init_acpi();
    printk("[ OK ]Started Interrupt controller.\n");
    enable_interrupts();
    // Caches
    printk("[WAIT] Configuring caches...\n");
    init_cache_cache();
    minos_socket_init_cache();
    init_epoll_cache();
    init_general_caches();
    init_charqueue();
    printk("[ OK ] Configured caches.\n");
    // Devices
    printk("[VERB] Loading PCI...\n");
    init_pci();
    // SMP
    printk("[VERB] Loading SMP...\n");
    init_smp();
    // Initialisation for process related things
    printk("[VERB] Configuring memory...\n");
    init_memregion();
    init_processes();
    init_tasks();
    init_kernel_task();
    init_schedulers();
    init_task_switch();
    init_resources();
    init_shm_cache();

    printk("[WAIT] Initilazing RTC...\n");
    disable_interrupts();
    calibrate_tsc();
    printk("[VERB] CPU calibrated at ~%llu Hz\n", tsc_hz);
    rtc_init();
    printk("[WAIT] Initilazed RTC.\n");
    rtc_print_time();

    printk("[VERB] Starting IDE...\n");
    ide_init();

    // VFS
    enable_interrupts();
    printk("[WAIT] Initilazing filesystms...\n");
    init_vfs();
    init_rootfs();
    printk("[ OK ] Initilazed filesystms.\n");
    printk("[VERB] Initilazing VFS devices...\n");
    init_devices();
    init_tty();

    printk("[VERB] Spawning init...\n");

    spawn_init();

    disable_interrupts();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    for(;;) {
        asm volatile("hlt");
    }
}
