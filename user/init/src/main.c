#include <minos/sysstd.h>
#include <minos/status.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    setenv("PATH", "/user:/sbin:", 0);
    intptr_t e = fork();
    const char* path = "/user/wm";
    if(e == (-YOU_ARE_CHILD)) {
        const char* argv[] = { path, NULL };
        if((e=execve(path, (char*const*) argv, (char*const*)environ)) < 0) {
            printf("ERROR: Failed to do exec: %s\n", status_str(e));
            exit(-e);
        }
        // Unreachable
        exit(0);
    } else if (e >= 0) {
        size_t pid = e;
        e=wait_pid(pid);
        if(e == NOT_FOUND) {
            printf("Could not find command `%s`\n", path);
        } else {
            printf("Child exited with: %d\n", (int)e);
        }
        exit(1);
    } else {
        printf("ERROR: fork %s\n",status_str(e));
        exit(1);
    }
    return 0;
}
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
