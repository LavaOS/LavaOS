#include <stdio.h>
#include <inttypes.h>
#include <minos/sysstd.h>
#include <minos/sysctl.h>

int main(void)
{
    char dnamebuf[MAX_SYSCTL_NAME];
    intptr_t dname = _sysctl(SYSCTL_DISTRO_NAME, dnamebuf);

    const char* hostname = getenv("HOSTNAME");

    char knamebuf[MAX_SYSCTL_NAME];
    intptr_t kname = _sysctl(SYSCTL_KERNEL_NAME, knamebuf);

    char karchbuf[MAX_SYSCTL_NAME];
    intptr_t karch = _sysctl(SYSCTL_KERNEL_ARCH, karchbuf);

    printf("%s %s %s %s\n", dnamebuf, hostname, knamebuf, karchbuf);
    return 0;
}
