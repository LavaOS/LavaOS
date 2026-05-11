#pragma once
#include <collections/list.h>
#include <sync/mutex.h>
typedef struct Task Task;
#include "scheduler.h"
#include <stdatomic.h>
#define MAX_PROCESSORS 65536
typedef struct {
    size_t lapic_ticks;
    Scheduler scheduler;
    Task* current_task;
    atomic_bool initialised;
} Processor;
