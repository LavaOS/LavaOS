/*++

Free95 20x/TX Kernel

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


Module Name:

    ntdll.h

Abstract:

    This module implements the functions to call the syscalls.

--*/

#ifndef NTDLL_H
#define NTDLL_H

#include "base.h"

void NtDisplayString(PUNICODE_STRING string)
{
    __asm__ __volatile__(
        "movl %0, %%eax\n\t"    // Load syscall number for NtDisplayString (0x25) into EAX
        "movl %1, %%ebx\n\t"    // Load the pointer to the string into EBX
        "int $0x2E\n\t"         // Trigger the syscall interrupt
        :
        : "r"(0x002e), "r"(string)
        : "eax", "ebx"
    );
}

int NtOpenFile(
    PHANDLE FileHandle,
    int DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PVOID IoStatusBlock,
    ULONG ShareAccess,
    ULONG OpenOptions
)
{
    int syscallResult = 0;

    asm volatile(
        "mov $0x004f, %%eax\n"
        "mov %1, %%edx\n"
        "int $0x2e\n"
        "mov %%eax, %0\n"
        : "=r" (syscallResult)
        : "r" (ObjectAttributes)
        : "eax", "edx"
    );

    return syscallResult;
}

#endif
