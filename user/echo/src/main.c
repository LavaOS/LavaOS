#include <minos/sysstd.h>
#include <stdio.h>

int main(int argc, const char** argv) {
    if (argc < 2) {
        printf("Usage: echo [text]\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (i > 1) printf(" ");
        printf("%s", argv[i]);
    }
    printf("\n");
    return 0;
}
