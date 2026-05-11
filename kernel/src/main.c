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

#include "fblogger.h"
#include "hash_table.h"
#include "kht.h"

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
    kinfo("Spawning `%s` id=%zu pid=%zu", epath, (size_t)e, ((Process*)(kernel.processes.items[(size_t)e]))->main_thread->id);
}
void _start() {
    disable_interrupts();
    BREAKPOINT();

    printk("Welcome to LavaOS!\n\n");

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

    init_fb_logger();

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
    printk("[WAIT] Starting Interrupt controller...\n");
    init_pic();
    init_acpi();
    printk("[ OK ] Started Interrupt controller.\n");
    enable_interrupts();
    printk("[WAIT] Configuring caches...\n");
    init_cache_cache();
    minos_socket_init_cache();
    init_epoll_cache();
    init_general_caches();
    init_charqueue();
    printk("[ OK ] Configured caches.\n");
    printk("[WAIT] Configuring and testing kernel hash table.\n");
    ht_init(&kernel_ht);
    printk("[ OK ] Kernel hash table is fine.\n");
    printk("[VERB] Loading PCI...\n");
    init_pci();
    printk("[VERB] Loading SMP...\n");
    init_smp();
    printk("[VERB] Initialazing load balancer lock...\n");
    spinlock_init(&kernel.load_balancer_lock);
    printk("[VERB] Configuring memory, starting core tasks and scheduler...\n");
    printk("[VERB] Memregion...\n");
    init_memregion();
    printk("[VERB] Processes...\n");
    init_processes();
    printk("[VERB] Tasks...\n");
    init_tasks();
    printk("[VERB] Kernel task...\n");
    init_kernel_task();
    printk("[VERB] Schedulers...\n");
    init_schedulers();
    printk("[VERB] Task switch...\n");
    init_task_switch();
    printk("[VERB] Resources...\n");
    init_resources();
    printk("[VERB] SHM Cache...\n");
    init_shm_cache();
    enable_interrupts();
    printk("[WAIT] Initilazing filesystms...\n");
    printk("[VERB] Initilazing VFS...\n");
    init_vfs();
    printk("[VERB] Initilazing rootfs...\n");
    init_rootfs();
    printk("[ OK ] Initilazed filesystms.\n");
    printk("[VERB] Initilazing devices...\n");
    init_devices();
    printk("[VERB] Initilazing TTY...\n");
    init_tty();

    spawn_init();

    disable_interrupts();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    for(;;) {
        asm volatile("hlt");
    }
}
