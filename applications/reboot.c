/*++

Free95

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


	PROJECT: Free95 Userspace Components
	FILE: reboot.c
	DESCRIPTION: A Native NT Application To Reboot The System.
	AUTHOR: @KapTheGuy

--*/

typedef enum _SHUTDOWN_ACTION
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION, *PSHUTDOWN_ACTION;

void _start()
{
    const unsigned long syscall_number = 0x00b4;
    const unsigned long shutdown_action = ShutdownReboot;
    int status;

    __asm__ __volatile__(
        "mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "int $0x2e\n"
        "mov %%eax, %0\n"
        : "=r"(status)
        : "r"(syscall_number), "r"(shutdown_action)
        : "eax", "ebx"
    );
}
