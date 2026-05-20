#pragma once
#include <stdint.h>
#include <minos/sysctl.h>

int hal_get_meminfo(SysctlMeminfo *out);
