#pragma once
#include "port.h"
#include "printk.h"

// Define KBC commands in power.h or a common header
#define KBC_PORT_COMMAND 0x64
#define KBC_PORT_DATA 0x60
#define KBC_RESET_CMD 0xFE
#define KBC_REBOOT_CMD 0xFE // Often the same as reset, or a specific command like 0x08
#define KBC_SELF_TEST_CMD 0xAA

// QEMU shutdown/reboot ports
#define QEMU_SHUTDOWN_PORT1 0xB0
#define QEMU_SHUTDOWN_PORT2 0xB1
#define QEMU_SHUTDOWN_CODE 0x2000
#define QEMU_REBOOT_CODE 0x2001

void powr_reboot() {
    printk("[POWR] Initiating system reboot...\n");

    // Method 1: Keyboard Controller Reset / Reboot Command
    printk("  Trying KBC Reboot Command (0x%X to 0x%X)...\n", KBC_REBOOT_CMD, KBC_PORT_COMMAND);
    outb(KBC_REBOOT_CMD, KBC_PORT_COMMAND);

    // Method 2: QEMU specific reboot port
    printk("  Trying QEMU reboot port (0x%X/0x%X)...\n", QEMU_SHUTDOWN_PORT1, QEMU_SHUTDOWN_PORT2);
    outw(QEMU_REBOOT_CODE, QEMU_SHUTDOWN_PORT1); // Use outw for 16-bit data
    outw(QEMU_REBOOT_CODE, QEMU_SHUTDOWN_PORT2);

    // Method 3: More detailed KBC interaction (ensure buffer is clear)
    printk("  Ensuring KBC is ready for more commands...\n");
    for (int i = 0; i < 10; ++i) { // Try a few times to clear buffer
        // status = inb(KBC_PORT_COMMAND); // Removed unused status variable
        if (!(inb(KBC_PORT_COMMAND) & 0x02)) { // If input buffer is empty
             outb(KBC_SELF_TEST_CMD, KBC_PORT_COMMAND); // Run self-test
             // status = inb(KBC_PORT_COMMAND); // Removed unused status variable
             if (!(inb(KBC_PORT_COMMAND) & 0x02)) { // If input buffer empty after self-test command
                  outb(KBC_REBOOT_CMD, KBC_PORT_COMMAND); // Send reboot command again
                  break; // Exit loop if successful
             }
        }
    }

    printk("[POWR] All attempted methods failed to trigger a reboot.\n");
    printk("       System may not respond to these commands or environment is incompatible.\n");
    kabort(); // Abort if reboot didn't happen
}

void powr_shutdown() {
    printk("[POWR] Initiating system shutdown...\n");

    // Method 1: QEMU specific shutdown port
    printk("  Trying QEMU shutdown port (0x%X/0x%X)...\n", QEMU_SHUTDOWN_PORT1, QEMU_SHUTDOWN_PORT2);
    outw(QEMU_SHUTDOWN_CODE, QEMU_SHUTDOWN_PORT1); // Use outw for 16-bit data
    outw(QEMU_SHUTDOWN_CODE, QEMU_SHUTDOWN_PORT2);

    printk("[POWR] All attempted methods failed to trigger a shutdown.\n");
    printk("       System may not respond to these commands or environment is incompatible.\n");
    kabort(); // Abort if shutdown didn't happen
}
