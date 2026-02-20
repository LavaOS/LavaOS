#include <stdio.h>
#include <string.h>
#include <minos/sysstd.h>
#include <minos/sysctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sizes.h>

#define KiB ((long)1024)
#define MiB ((long)1024 * KiB)
#define GiB ((long)1024 * MiB)
#define TiB ((long)1024 * GiB)

int main() {
    char namebuf[MAX_SYSCTL_NAME];
    intptr_t e = _sysctl(SYSCTL_KERNEL_NAME, namebuf);
    if(e < 0) strcpy(namebuf, "Unknown");
    SysctlMeminfo meminfo = { 0 };
    e = _sysctl(SYSCTL_MEMINFO, &meminfo);
    if(e < 0) {
        meminfo.total = 0;
        meminfo.free = 0;
        meminfo.used = 0;
    }
    printf("Total: %ld MiB\nUsed: %ld MiB\nFree: %ld MiB\nShared: %ld MiB\nBuff/Cache: %ld MiB\nAvailable: %ld MiB\n", meminfo.total / MiB, meminfo.used / MiB, meminfo.free / MiB, 0, 0, meminfo.free / MiB);
}
