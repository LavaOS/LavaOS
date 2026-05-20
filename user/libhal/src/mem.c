#include "../include/hal/mem.h"
#include <minos/sysstd.h>
#include <errno.h>
#include <string.h>

int hal_get_meminfo(SysctlMeminfo *out)
{
    if (!out) {
        errno = EINVAL;
        return -1;
    }

    memset(out, 0, sizeof(*out));

    intptr_t e = _sysctl(SYSCTL_MEMINFO, out);
    if (e < 0) {
        errno = (int)-e;
        return -1;
    }

    return 0;
}
