/*++

Free95

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


	PROJECT: Free95 Userspace Components
	FILE: print.h
	DESCRIPTION: Simple library to abstract NT Syscalls
	AUTHORS: @KapTheGuy, @realblobii

--*/


#ifndef PRINT_H
#define PRINT_H

typedef struct _UNICODE_STRING
{
    unsigned short Length;
    unsigned short MaximumLength;
    char* Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef unsigned long NTSTATUS;

NTSTATUS NtDisplayString(PUNICODE_STRING String);
void RtlCreateUnicodeStringFromAsciiz(PUNICODE_STRING UnicodeString, const char* SourceString);
void RtlCliDisplayString(const char *msg);
void RtlCreateStringFromUint(unsigned int value, char *str);

#endif
