#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    const char* user = getenv("USER");
    if (!user || !user[0]) {
        write(STDERR_FILENO, "whoami: USER not set\n", 21);
        return 1;
    }
    write(STDOUT_FILENO, user, strlen(user));
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}
