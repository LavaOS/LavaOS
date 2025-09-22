#include <stdio.h>

int main() {
    printf("\033[2J\033[H");
    fflush(stdout);
}
