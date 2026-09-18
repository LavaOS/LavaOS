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
#include "hpet.h"

#include "fblogger.h"
#include "hash_table.h"
#include "kht.h"

static void set_version() {
    kernel.kname = KNAME;
    kernel.karch = "x86_64";

    kernel.dname = DNAME;
    kernel.dcode = DCODE;
    kernel.dver = DVER;
}
void spawn_init(void) {
    intptr_t e = 0;
    const char* epath = NULL;
    Args args;
    Args env;
    epath = "/init";
    const char* argv[] = {epath, NULL};
    args = create_args(argv);
    const char* envv[] = {NULL};
    env  = create_args(envv);
    if((e = exec_new(epath, &args, &env)) < 0) kpanic("Failed to exec %s : %s",epath,status_str(e));
}
void _start() {
    disable_interrupts();
    BREAKPOINT();
    enable_interrupts();

    set_version();

    printk("Using %s kernel.\n", kernel.kname);
    printk("Now booting %s v%s (%s).\n", kernel.dname, kernel.dver, kernel.dcode);

    printk("\n");

    printl_wait("Initializing serial...\n");
    serial_init();

    kernel.logger = &serial_logger;
    kernel.logger->level = LOG_ALL;

    printl_ok("Initialized serial.\n");
    printl_wait("Initializing cmdline...\n");
    init_cmdline();
    printl_ok("Initialized cmdline.\n");
    printl_wait("Initializing loggers...\n");
    init_loggers();
    init_fb_logger();
    printl_ok("Initialized loggers.\n");
    printl_wait("Initializing GDT and IDT...\n");
    init_gdt();
    disable_interrupts();
    init_idt();
    enable_interrupts();
    printl_ok("Initialized GDT and IDT.\n");
    printl_wait("Initializing essential components and devices...\n");
    init_exceptions();
    reload_tss();
    init_bitmap();
    init_paging();
    KERNEL_SWITCH_VTABLE();
    enable_cpu_features();
    printl_ok("Initialized essential components and devices.\n");
    printl_wait("Starting Interrupt controller...\n");
    init_pic();
    init_acpi();
    printl_ok("Started Interrupt controller.\n");
    printl_verb("Initializing HPET...\n");
    hpet_init();
    enable_interrupts();
    printl_verb("Configuring caches...\n");
    init_cache_cache();
    minos_socket_init_cache();
    init_epoll_cache();
    init_general_caches();
    init_charqueue();
    printl_verb("Configuring and testing kernel hash table.\n");
    ht_init(&kernel_ht);
    printl_wait("Loading PCI...\n");
    init_pci();
    printl_ok("PCI OK.\n");
    printl_wait("Loading SMP...\n");
    init_smp();
    printl_ok("SMP OK.\n");
    printl_verb("Initializing load balancer lock...\n");
    spinlock_init(&kernel.load_balancer_lock);
    printl_verb("Memregion...\n");
    init_memregion();
    printl_verb("Processes...\n");
    init_processes();
    printl_verb("Tasks...\n");
    init_tasks();
    printl_verb("Kernel task...\n");
    init_kernel_task();
    printl_verb("Schedulers...\n");
    init_schedulers();
    printl_verb("Task switch...\n");
    init_task_switch();
    printl_verb("Resources...\n");
    init_resources();
    printl_verb("SHM Cache...\n");
    init_shm_cache();
    enable_interrupts();
    printl_verb("Initializing VFS...\n");
    init_vfs();
    printl_verb("Initializing rootfs...\n");
    init_rootfs();
    printl_verb("Initializing devices...\n");
    init_devices();
    printl_verb("Initializing TTY...\n");
    init_tty();

    spawn_init();

    disable_interrupts();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    for(;;) asm volatile("hlt");
}
