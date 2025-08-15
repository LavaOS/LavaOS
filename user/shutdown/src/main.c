#include <stdint.h>
#include <sys/io.h>

int main() {
    outw(0xF4, 0x0000);
}
