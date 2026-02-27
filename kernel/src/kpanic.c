#include "kpanic.h"
#include "serial.h"
#include "log.h"
#include "printk.h"
#include "shell/sh.h"
#include <stdarg.h>

/* void kpanic(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);

    kernel.logger = &serial_logger;
    kfatal_va(fmt, args);

    va_end(args);

    va_start(args, fmt);

    kclear(0x8B0000);
    printk_set_color(0xFFFFFF, 0x8B0000);

    printk("\n\n");

    printk("  ###          ###  \n");
    printk("  ###         ###   \n");
    printk("  ###        ###    +---------------------------------------------+\n");
    printk("            ###     |LavaOS has encountered a critical problem.   |\n");
    printk("           ###      |The system has been halted to prevent damage.|\n");
    printk("          ###       |You can safely power off your computer.      |\n");
    printk("  ###    ###        +---------------------------------------------+\n");
    printk("  ###   ###         \n");
    printk("  ###  ###          \n");

    printk("\n");

    printk("  Error: ");
    vprintk(fmt, args);
    printk("\n\n");

    printk("  If you wont, you can create an issuse on GithHub to get help about this error.\n");
    printk("  GitHub: https://github.com/LavaOS/LavaOS/\n");

    //TODO: Error code

    printk_reset_color();

    va_end(args);

    kabort();
} */

void kpanic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kernel.logger = &serial_logger;
    kfatal_va(fmt, args);
    va_end(args);
    va_start(args, fmt);
    kclear(0x000000);
    printk_set_color(0xFF0000, 0x000000);
    printk("\n\n");
    printk("  !!! KERNEL PANIC !!!\n\n");
    printk("  Error: ");
    vprintk(fmt, args);
    printk("\n\n");
    printk("  System entered recovery mode.\n");
    printk("  Type 'help' for commands.\n\n");
    printk_reset_color();
    va_end(args);
    kernel_shell_run();
    for (;;)
        __asm__ volatile("hlt");
}
