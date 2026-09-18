#include <minos/sysstd.h>
#include <stdio.h>

int main(int argc, const char** argv) {
    if (argc < 2) {
        printf("Usage: mkdir [path]\n");
        return 1;
    }
    if (argc > 2) {
        printf("Warning: Too much arguments!\n");
    }

    syscall1(SYS_MKDIR, argv[1]);
    return 0;
}
