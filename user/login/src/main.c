#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_INPUT 64

static void read_line(char* buf, size_t max) {
    size_t i = 0;
    char c;

    while (i + 1 < max) {
        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == '\n' || c == '\r')
            break;

        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                write(STDOUT_FILENO, "\b \b", 3);
            }
            continue;
        }

        buf[i++] = c;
        write(STDOUT_FILENO, &c, 1);
    }

    buf[i] = 0;
    write(STDOUT_FILENO, "\n", 1);
}

static void read_password(char* buf, size_t max) {
    size_t i = 0;
    char c;

    while (i + 1 < max) {
        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == '\n' || c == '\r')
            break;

        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                write(STDOUT_FILENO, "\b \b", 3);
            }
            continue;
        }

        buf[i++] = c;

        write(STDOUT_FILENO, "\b \b", 3);
    }

    buf[i] = 0;
    write(STDOUT_FILENO, "\n", 1);
}

int main(void) {
    char username[MAX_INPUT];
    char password[MAX_INPUT];

    const char* hostname = getenv("HOSTNAME");

    while (true) {
        write(STDOUT_FILENO, hostname, strlen(hostname));
        write(STDOUT_FILENO, " login: ", 8);
        read_line(username, sizeof(username));

        write(STDOUT_FILENO, "password: ", 10);
        read_password(password, sizeof(password));

        if (strcmp(username, "root") == 0 &&
            strcmp(password, "root") == 0) {

            write(STDOUT_FILENO, "\nWelcome, ", 10);
            write(STDOUT_FILENO, username, strlen(username));
            write(STDOUT_FILENO, "!\n\n", 3);

            setenv("USER", username, 2);

            const char* path = "/sbin/lash";
            char* const argv[] = { (char*)path, NULL };

            execve(path, argv, environ);

            write(STDOUT_FILENO,
                  "login: failed to start shell\n",
                  31);
            exit(1);
        }

        write(STDOUT_FILENO,
              "Login incorrect\n\n",
              17);
    }
}
