#include <minos/sysstd.h>
#include <minos/status.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void _start(int argc, const char** argv, const char** envp) {
    intptr_t e;
    if((e = open("/devices/tty0", O_RDWR)) < 0) {
        exit(-e); 
    }
    _libc_init_environ(envp);
    _libc_init_streams();
    fprintf(stderr, "\033[2J\033[H");
    int code = main();
    free(STDOUT_FILENO);
    close(STDOUT_FILENO);
    if(STDIN_FILENO != STDOUT_FILENO) {
        free(STDIN_FILENO);
        close(STDIN_FILENO);
    }
    free(code);
    exit(code);
}
int main() {
    setenv("PATH", "/user:/sbin:", 0);

    const char* path = "/user/wm";

    while (1) {
        intptr_t e = fork();

        if (e == (-YOU_ARE_CHILD)) {
            const char* argv[] = { path, NULL };
            e = execve(path, (char*const*)argv, (char*const*)environ);

            printf("ERROR: exec failed: %s\n", status_str(e));
            exit(-e);
        }

        else if (e >= 0) {
            size_t pid = e;
            intptr_t code = wait_pid(pid);

            printf("\n[init] child exited with code %d ---- restarting...\n\n", code);
        }

        else {
            printf("ERROR: fork %s\n", status_str(e));
            exit(1);
        }
    }

    return 0;
}
