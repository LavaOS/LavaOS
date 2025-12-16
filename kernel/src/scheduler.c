#include "scheduler.h"
#include "task.h"
#include "kernel.h"

static struct list scheduler_ready_queue;
static Mutex scheduler_mutex = {0};

static void scheduler_init(Scheduler* scheduler) {
    list_init(&scheduler_ready_queue);
    list_init(&scheduler->queue);
    // mutex_init(&scheduler_mutex);
}
void init_schedulers(void) {
    for(size_t i = 0; i < kernel.max_processor_id+1; ++i) {
        scheduler_init(&kernel.processors[i].scheduler);
    }
}
Task* task_select(Scheduler* scheduler) {
    Task* task = NULL;
    mutex_lock(&scheduler->queue_mutex);
    if(list_empty(&scheduler->queue)) {
        mutex_unlock(&scheduler->queue_mutex);
        return NULL;
    }
    size_t n = list_len(&scheduler->queue);
    for(size_t i = 0; i < n; task=NULL, ++i) {
        task = (Task*)scheduler->queue.next;
        list_remove(&task->list);
        // Move it to the back
        list_insert(&task->list, &scheduler->queue);
        if(task->flags & TASK_FLAG_BLOCKING) {
            debug_assert(task->blocker.try_resolve);
            if(task->blocker.try_resolve(&task->blocker, task)) {
                task->flags &= ~TASK_FLAG_BLOCKING;
                break;
            }
            continue;
        }
        break;
    }
    mutex_unlock(&scheduler->queue_mutex);
    return task;
}

void scheduler_enqueue(Task* task) {
    assert(task);

    mutex_lock(&scheduler_mutex);

    if (task->state == TASK_READY) {
        mutex_unlock(&scheduler_mutex);
        return;
    }

    task->state = TASK_READY;
    list_insert(&scheduler_ready_queue, &task->sched_list);

    mutex_unlock(&scheduler_mutex);
}
