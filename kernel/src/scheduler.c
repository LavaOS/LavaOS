#include "scheduler.h"
#include "task.h"
#include "kernel.h"
#include <sync/spinlock.h>

static struct list scheduler_ready_queue;

static void scheduler_init(Scheduler* scheduler) {
    list_init(&scheduler_ready_queue);
    list_init(&scheduler->queue);
}

void init_schedulers(void) {
    for(size_t i = 0; i < kernel.max_processor_id + 1; ++i) {
        scheduler_init(&kernel.processors[i].scheduler);
    }
    spinlock_init(&kernel.load_balancer_lock); 
}

void scheduler_enqueue(Task* task) {
    assert(task);

    spinlock_lock(&kernel.load_balancer_lock);

    if (task->state == TASK_READY) {
        spinlock_unlock(&kernel.load_balancer_lock);
        return;
    }

    task->state = TASK_READY;
    list_insert(&scheduler_ready_queue, &task->sched_list);

    spinlock_unlock(&kernel.load_balancer_lock);
}

Task* task_select(Scheduler* scheduler) {
    Task* task = NULL;
    
    spinlock_lock(&kernel.load_balancer_lock); 

    if(list_empty(&scheduler->queue)) {
        spinlock_unlock(&kernel.load_balancer_lock); 
        return NULL;
    }
    size_t n = list_len(&scheduler->queue);
    for(size_t i = 0; i < n; task=NULL, ++i) {
        task = (Task*)scheduler->queue.next;
        list_remove(&task->list);
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
    spinlock_unlock(&kernel.load_balancer_lock); 
    return task;
}
