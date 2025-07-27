#include <stdint.h>

// Inline assembly to write to IO port
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

int main() {
    outw(0x604, 0x2000); // ACPI shutdown for QEMU
    return 0;
}
