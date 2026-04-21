#include "kpanic.h"
#include "serial.h"
#include "log.h"
#include "printk.h"
#include "shell/sh.h"
#include <stdarg.h>

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

    printk("  +--------------------------------------------+\n");
    printk("  | LavaOS has encountered a critical problem. |\n");
    printk("  | System will enter recovery mode.           |\n");
    printk("  | Type 'help' for a list of commands.        |\n");
    printk("  +--------------------------------------------+\n");

    printk("\n");

    printk("  Error: ");
    vprintk(fmt, args);
    printk("\n\n");

    printk_reset_color();
    va_end(args);
    kernel_shell_run();
    kabort();
}
