#include "kpanic.h"
#include "serial.h"
#include "log.h"
#include "printk.h"
#include <stdarg.h>

void kpanic(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    kernel.logger = &serial_logger;
    kfatal_va(fmt, args);

    va_end(args);

    va_start(args, fmt);

    kclear(0xAA0000);
    printk_set_color(0xFFFFFF, 0xAA0000);

    printk("\n\n");
    printk("  LavaOS has encountered a critical problem.\n");
    printk("  The system has been halted to prevent damage.\n\n");

    printk("  You can safely power off your computer.\n\n");

    printk("  Error: ");
    vprintk(fmt, args);
    printk("\n\n");

    printk_reset_color();

    va_end(args);

    kabort();
}
