#include <stdint.h>
#include <sys/io.h>

int main() {
    outb(0x64, 0xFE);
    return 0;
}
