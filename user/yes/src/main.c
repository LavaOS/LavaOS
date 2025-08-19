#include <minos/sysstd.h>
#include <minos/status.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

const char* shift_args(int *argc, const char ***argv) {
    if((*argc) <= 0) return NULL;
    const char* arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}

int main(int argc, const char** argv) {
    const char* arg;
    const char* exe = shift_args(&argc, &argv);  // Skip program name
    assert(exe && "Expected exe. Found nothing");

    if (argc == 0) {
        printf("Usage: yes [text]\n");
        return 1;
    }

    int first = 1;
    while((arg = shift_args(&argc, &argv))) {
        if (!first) printf(" ");
	while(1) {
            printf("%s", arg);
	}
        first = 0;
    }

    printf("\n");
    return 0;
}
