#include <minos/sysstd.h>
#include <stdio.h>

int main(int argc, const char** argv) {
    if (argc < 2) {
        printf("Usage: yes [text]\n");
        return 1;
    }

    const char* text = argv[1];
    while (1) {
        printf("%s\n", text);
    }

    return 0;
}
