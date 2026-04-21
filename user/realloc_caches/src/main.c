#include <minos/sysstd.h>
#include <minos/syscodes.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, const char** argv) {
    syscall0(SYS_REALLOC_CACHES);
    return 0;
}
