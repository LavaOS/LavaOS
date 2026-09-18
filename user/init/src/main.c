#include <minos/sysstd.h>
#include <minos/status.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(void);

ssize_t readline(int fd, char *buf, size_t size)
{
    size_t pos = 0;

    if (!buf || size == 0)
        return -1;

    while (pos < size - 1) {
        char c;
        ssize_t n = read(fd, &c, 1);

        if (n < 0)
            return n;

        if (n == 0)
            break;

        if (c == '\n')
            break;

        buf[pos++] = c;
    }

    buf[pos] = '\0';

    return (ssize_t)pos;
}

void _start(int argc, const char** argv, const char** envp) {
    const char* std = "/devices/tty0";
    if(open(std, O_RDONLY) < 0 ||   /*STDIN  (fd 0)*/
       open(std, O_WRONLY) < 0 ||   /*STDOUT (fd 1)*/
       open(std, O_WRONLY) < 0) {   /*STDERR (fd 2)*/
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

#define MAX_SESSION_RESTARTS 5

int main(void) {
    printf("\033[2J\033[H");
    fflush(stdout);

    int sesfile = open("/syscfg/session", O_RDONLY);
    int hfile = open("/syscfg/hostname", O_RDONLY);

    if (sesfile < 0) {
        fprintf(stderr, "Failed to open session\n");
        return 1;
    }
    if (hfile < 0) {
        fprintf(stderr, "Failed to open hostname\n");
        return 1;
    }

    char sesline[256];
    char hline[256];

    if (readline(sesfile, sesline, sizeof(sesline)) < 0) {
        fprintf(stderr, "Failed to read session\n");
        close(sesfile);
        return 1;
    }
    if (readline(hfile, hline, sizeof(hline)) < 0) {
        fprintf(stderr, "Failed to read hostname\n");
        close(hfile);
        return 1;
    }

    printf("[INIT] Setting environment...\n");
    setenv("PATH", "/user:/sbin:", 0);
    setenv("HOSTNAME", hline, 1);
    setenv("SESSION", sesline, 1);

    close(sesfile);
    close(hfile);

    const char* services[] = {
        "/etc/init.d/login",
    };
    size_t num_services = sizeof(services) / sizeof(services[0]);

    int restarts = 0;

    for(;;) {
        printf("[INIT] Starting services...\n");

        intptr_t pids[16];
        size_t running = 0;

        for(size_t i = 0; i < num_services && running < 16; ++i) {
            intptr_t pid = fork();
            if(pid == 0) {
                const char* argv[] = { services[i], NULL };
                execve(services[i], (char*const*)argv, (char*const*)environ);
                printf("[INIT] Failed to exec %s\n", services[i]);
                exit(1);
            }
            if(pid < 0) {
                printf("[INIT] fork failed for %s\n", services[i]);
                continue;
            }
            pids[running++] = pid;
            printf("[INIT] Started %s (pid %ld)\n", services[i], (long)pid);
        }

        if(running == 0) {
            printf("[INIT] No services to run. Halting.\n");
            for(;;);
        }

        printf("[INIT] Waiting for services... (%zu running)\n", running);

        for(size_t i = 0; i < running; ++i) {
            intptr_t code = wait_pid(pids[i]);
            printf("[INIT] Service %zu exited with code %ld\n", i, (long)code);
        }

        restarts++;
        if(restarts >= MAX_SESSION_RESTARTS) {
            printf("[INIT] Too many restarts (%d). System halted.\n", restarts);
            for(;;);
        }

        printf("[INIT] Session ended. Restarting in 2 seconds... (attempt %d/%d)\n",
               restarts, MAX_SESSION_RESTARTS);

        // Brief delay before restart (busy-wait since sleep may not be available)
        for(volatile int i = 0; i < 20000000; i++) {}

        printf("\033[2J\033[H");
        fflush(stdout);
    }
}
