#include <minos/sysstd.h>
#include <minos/status.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void _start(int argc, const char** argv, const char** envp) {
    fprintf(stderr, "\033[2J\033[H");
    const char* std = "/devices/tty0";
    if(open(std, O_WRONLY) < 0 ||   /*STDOUT*/
       open(std, O_RDONLY) < 0 ||   /*STDIN*/
       open(std, O_WRONLY) < 0) {   /*STDERR*/
        exit(1); 
    }
    _libc_init_environ(envp);
    _libc_init_streams();
    int code = main();
    close(STDOUT_FILENO);
    if(STDIN_FILENO != STDOUT_FILENO) {
        close(STDIN_FILENO);
    }
    exit(code);
}
int main() {
    printf("\033[2J\033[H");
    fflush(stdout);

    printf("[INIT] '/sbin/init' started!\n");

    printf("[INIT] Setting path...\n");
    setenv("PATH", "/user:/sbin:", 0);
    printf("[INIT] Setting hostname...\n");
    setenv("HOSTNAME", "lavaos", 1);

    printf("\033[2J\033[H");
    fflush(stdout);

    const char* path = "/etc/init.d/login";
    const char* argv[] = { path, NULL };

    intptr_t pid = fork();
    if (pid == 0) {
        execve(path, (char*const*)argv, (char*const*)environ);
        printf("[INIT] Child returned.\n");
        exit(1);
    }

    if (pid < 0) {
        printf("[INIT] fork failed\n");
        exit(1);
    }

    intptr_t code = wait_pid(pid);
    printf("\n[INIT] Session ended (code %ld)\n", code);

    printf("[INIT] System halted.\n");
    for (;;);
}
