#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>  // execve
extern char **environ;

#define MAX_INPUT 64

static void read_input(char* buf, size_t max, bool hide) {
    size_t i = 0;
    char c;

    while (i + 1 < max) {
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        if (c == '\n' || c == '\r') break;

        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                write(STDOUT_FILENO, "\b \b", 3);
            }
            continue;
        }

        buf[i++] = c;

        if (hide) write(STDOUT_FILENO, "\b \b", 3);
        else       write(STDOUT_FILENO, &c, 1);
    }

    buf[i] = 0;
    write(STDOUT_FILENO, "\n", 1);
}

int main(void) {
    char username[MAX_INPUT];
    char password[MAX_INPUT];

    const char* hostname = getenv("HOSTNAME");
    if (!hostname) hostname = "lavaos";

    while (true) {
        write(STDOUT_FILENO, hostname, strlen(hostname));
        write(STDOUT_FILENO, " login: ", 8);
        read_input(username, sizeof(username), false);

        write(STDOUT_FILENO, "password: ", sizeof("password: ") - 1);
        read_input(password, sizeof(password), true);

        if (strcmp(username, "root") == 0 &&
            strcmp(password, "root") == 0) {

            write(STDOUT_FILENO, "\nWelcome, ", 10);
            write(STDOUT_FILENO, username, strlen(username));
            write(STDOUT_FILENO, "!\n\n", 3);

            setenv("USER", username, 2);

            const char* path = "/user/wm";
            char* const argv[] = { (char*)path, NULL };

            execve(path, argv, environ);

            perror("failed to start shell");
            /* write(STDOUT_FILENO,
                  "login: failed to start shell\n",
                  31); */
            memset(password, 0, sizeof(password));
            exit(1);
        }

        write(STDOUT_FILENO,
              "Login incorrect\n\n",
              17);
    }
}
