#include <minos/sysstd.h>
#include <minos/status.h>
#include <stdio.h>

int main(int argc, const char** argv) {
    if (argc < 2) {
        printf("Usage: mkdir [path]\n");
        return 1;
    }
    if (argc > 2) {
        printf("Warning: Too much arguments!\n");
    }

    intptr_t result = syscall1(SYS_MKDIR, argv[1]);
    if (result < 0) {
        printf("mkdir: cannot create directory '%s': %s\n", argv[1], status_str(result));
        return 1;
    }
    return 0;
}
