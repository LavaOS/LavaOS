#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    const char* session = getenv("SESSION");
    if (!session || !session[0]) {
        write(STDOUT_FILENO, "Unknown", 7);
        write(STDOUT_FILENO, "\n", 1);
        return 0;
    }
    write(STDOUT_FILENO, session, strlen(session));
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}
