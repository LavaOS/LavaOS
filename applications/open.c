/*++

Free95

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


	PROJECT: Free95 Userspace Components
	FILE: program.c
	DESCRIPTION: A simple Native NT Application to determine if a file exists or not.
	AUTHOR: @KapTheGuy

--*/

#include "appinclude/print.h"

typedef struct _OBJECT_ATTRIBUTES
{
    unsigned long Length;
    void* RootDirectory;
    PUNICODE_STRING ObjectName;
    unsigned long Attributes;
    void* SecurityDescriptor;
    void* SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

int NtOpenFile(
    void** FileHandle,
    int DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    void* IoStatusBlock,
    unsigned long ShareAccess,
    unsigned long OpenOptions
);

void _start()
{
	char fnamebuf[] = "0:/boot.ini";

	PUNICODE_STRING fname = 0;
	
	RtlCreateUnicodeStringFromAsciiz(fname, fnamebuf);

	POBJECT_ATTRIBUTES objAttrs;

	objAttrs->ObjectName = fname;

	unsigned long syscallResult;

	asm volatile(
        "mov $0x004f, %%eax\n"
        "mov %1, %%edx\n"
        "int $0x2e\n"
        "mov %%eax, %0\n"
        : "=r" (syscallResult)
        : "r" (objAttrs)
        : "eax", "edx"
    );

	NtOpenFile(0, 0, objAttrs, 0, 0, 0);

	NtDisplayString(fname);

	if (syscallResult == 0)
	{
		RtlCliDisplayString(" File exists\n");
	}
	else
	{
		RtlCliDisplayString(" File does not exist\n");
	}
}

int NtOpenFile(
    void** FileHandle,
    int DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    void* IoStatusBlock,
    unsigned long ShareAccess,
    unsigned long OpenOptions
)
{
	int r = 0;

	asm volatile(
        "mov $0x004f, %%eax\n"
        "mov %1, %%edx\n"
        "int $0x2e\n"
        "mov %%eax, %0\n"
        : "=r" (r)
        : "r" (ObjectAttributes)
        : "eax", "edx"
    );

	return r;
}
