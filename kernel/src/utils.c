#include "utils.h"
#include <stddef.h>
#include "assert.h"
#include "arch/x86_64/idt.h"
#include "devices/ps2/keyboard/keyboard.h"

void kabort(void) {
    // disable_interrupts();
    // asm volatile ("cli");
    
    while(1) {
        // off
        ps2_set_capslock(true);
        for(volatile uint64_t i = 0; i < 10000000; i++) {
            asm volatile ("pause");
        }
        
        // off
        ps2_set_capslock(false);
        for(volatile uint64_t i = 0; i < 10000000; i++) {
            asm volatile ("pause");
        }
    }
}
