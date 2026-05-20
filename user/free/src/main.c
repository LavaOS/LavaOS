#include <stdio.h>
#include <inttypes.h>
#include <sizes.h>
#include <hal/core.h>
#include <hal/mem.h>

int main(void)
{
    SysctlMeminfo sys_mem;

    if (hal_get_meminfo(&sys_mem) < 0) {
        printf("Failed to get memory info\n");
        return 1;
    }

    printf("Total: %" PRIu64 " MiB\n", sys_mem.total / MiB);
    printf("Used: %" PRIu64 " MiB\n", sys_mem.used / MiB);
    printf("Free: %" PRIu64 " MiB\n", sys_mem.free / MiB);
    printf("Shared/Cache: %" PRIu64 " MiB\n", sys_mem.cached / MiB);
    printf("Available: %" PRIu64 " MiB\n", sys_mem.available / MiB);

    return 0;
}
