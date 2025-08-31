/*++

Free95

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


	PROJECT: Free95 Userspace Components
	FILE: lsbin.c
	DESCRIPTION: Lists all programs available from a (hardcoded) list. Will update ot be dynamic in future.
	AUTHOR: @realblobii, @KapTheGuy

--*/

#include "appinclude/print.h"

const char* apps[] =
{
    "reboot.exe", "stop.exe", "hello.exe", 
    "open.exe", "lsbin.exe", "freever.exe", "bsod.exe"
};

void _start()
{
	for (int i = 0; i < sizeof(apps) / sizeof(apps[0]); i++)
	{
        RtlCliDisplayString(apps[i]);  
        RtlCliDisplayString("\n");
	}
}
