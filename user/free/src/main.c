#include <stdio.h>
#include <string.h>
#include <minos/sysstd.h>
#include <minos/sysctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sizes.h>

int main() {
    char namebuf[MAX_SYSCTL_NAME];
    intptr_t e = _sysctl(SYSCTL_KERNEL_NAME, namebuf);
    if(e < 0) strcpy(namebuf, "Unknown");
    SysctlMeminfo meminfo = { 0 };
    e = _sysctl(SYSCTL_MEMINFO, &meminfo);
    if(e < 0) {
        meminfo.total = 0;
        meminfo.free = 0;
        meminfo.cached = 0;
        meminfo.used = 0;
        meminfo.available = 0;
    }
    printf("Total: %ld MiB\nUsed: %ld MiB\nFree: %ld MiB\nShared/Cache: %ld MiB\nAvailable: %ld MiB\n", meminfo.total / MiB, meminfo.used / MiB, meminfo.free / MiB, meminfo.cached / MiB, meminfo.available / MiB);
}
