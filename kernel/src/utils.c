#include "utils.h"
#include <stddef.h>
#include "assert.h"
#include "arch/x86_64/idt.h"

// TODO: Better error reporting
void kabort(void) {
    disable_interrupts();
    asm volatile ("cli; hlt");
    while(1) {
        asm volatile ("hlt");
    }
}
