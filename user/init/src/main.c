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
    printf("[VERB] Setting path...\n");
    setenv("PATH", "/user:/sbin:", 0);
    printf("[VERB] Setting hostname...\n");
    setenv("HOSTNAME", "lavaos", 1);

    const char* path = "/etc/init.d/login";
    const char* argv[] = { path, NULL };

    printf("\033[2J\033[H");
    fflush(stdout);

    intptr_t pid = fork();
    if (pid == (-YOU_ARE_CHILD)) {
        execve(path, (char*const*)argv, (char*const*)environ);
        printf("init: exec failed\n");
        exit(1);
    }

    if (pid < 0) {
        printf("init: fork failed\n");
        exit(1);
    }

    intptr_t code = wait_pid(pid);
    printf("\n[init] session ended (code %ld)\n", code);

    printf("[init] system halted\n");
    for (;;);
}
