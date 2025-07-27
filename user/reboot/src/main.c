#include <stdint.h>

// Inline assembly to write to IO port
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

int main() {
    outb(0x64, 0xFE); // Reboot using keyboard controller
    return 0;
}
