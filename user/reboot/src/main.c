#include <minos/syscall.h>
#include <minos/syscodes.h>

int main() {
    syscall0(SYS_REBOOT);
}
