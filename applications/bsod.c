/*++

Free95

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


	PROJECT: Free95 Userspace Components
	FILE: bsod.c
	DESCRIPTION: A Native NT Application To Trigger A BSoD.
	AUTHOR: @KapTheGuy

--*/

#include "appinclude/print.h"

void _start()
{
	RtlCliDisplayString("Crashing your system!");
    asm volatile("hlt"); // Should trigger a GPF
}
