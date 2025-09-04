/*++

Free95 20x/TX Kernel

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


Module Name:

    ntdll.c

Abstract:

    This module implements replicating NTDLL, an essential Dynamic Link Library for Windows Applications.
    It implements low-level functions, like invoking syscalls.

--*/

#include <windows.h>
#include <stdio.h>

typedef struct _UNICODE_STRING
{
    unsigned short Length;
    unsigned short MaximumLength;
    char* Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

int NtDisplayString(PUNICODE_STRING String);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	PUNICODE_STRING fname = 0;
	fname->Length = 30;
	fname->MaximumLength = 50;
	fname->Buffer = "Hello, DLL!\n";
	
	NtDisplayString(fname);

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        	fname->Buffer = "Process attached!\n";
        	NtDisplayString(fname);
            break;
        case DLL_THREAD_ATTACH:
        	fname->Buffer = "Thread attached!\n";
        	NtDisplayString(fname);
            break;
        case DLL_THREAD_DETACH:
            // Code to run when a thread is destroyed
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

__declspec(dllexport) int AddNumbers(int a, int b) {
    return a + b;
}

int NtDisplayString(PUNICODE_STRING String)
{
	asm volatile (
					"movl $0x002e, %%eax\n\t"
					"movl %0, %%ebx\n\t"
					"int $0x2e\n\t"
					:
					: "r"(String)
					: "%eax", "%ebx"
			);

    return 0;
}
