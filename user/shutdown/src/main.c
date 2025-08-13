#include <stdint.h>
#include <io.h>

int main() {
    outw(0x604, 0x2000);
    return 0;
}
