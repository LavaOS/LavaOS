#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

extern char **environ;

#define MAX_INPUT 64

static void read_input(char* buf, size_t max) {
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
    }
    buf[i] = 0;
}

static void print_file(const char *path)
{
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        printf("[LGIN] Failed to open %s\n", path);
        return;
    }

    char buf[256];
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, n);
    }

    close(fd);
}

int main(void) {
    char username[MAX_INPUT];
    char password[MAX_INPUT];

    const char* hostname = getenv("HOSTNAME");
    if (!hostname) hostname = "(none)";

    while (true) {
        write(STDOUT_FILENO, hostname, strlen(hostname));
        write(STDOUT_FILENO, " login: ", 8);
        read_input(username, sizeof(username));

        write(STDOUT_FILENO, "password: ", sizeof("password: ") - 1);
        read_input(password, sizeof(password));

        if (strcmp(username, "root") == 0 &&
            strcmp(password, "root") == 0) {

            setenv("USER", username, 2);


            const char* path;
            const char* session = getenv("SESSION");

            if (!session) {
                printf("[LGIN] Session is empty\n");
                exit(1);
            } else if (strcmp(session, "desktop") == 0) {
                path = "/user/wm";
            } else if (strcmp(session, "tty") == 0) {
                path = "/sbin/lash";
            } else {
                printf("[LGIN] Invaild session\n");
                exit(1);
            }
            char* const argv[] = { (char*)path, NULL };

            pid_t pid = fork();

            if (pid == 0) {
                char *argv[] = { "/user/cat", "/syscfg/motd", NULL };
                execve("/user/cat", argv, environ);
                _exit(127);
            }

            if (pid > 0)
                waitpid(pid, NULL, 0);
            
            execve(path, argv, environ);

            printf("[LGIN] Failed to start child\n");
            memset(password, 0, sizeof(password));
            exit(1);
        }

        write(STDOUT_FILENO, "[LGIN] Login incorrect\n\n", 24);
    }
}
