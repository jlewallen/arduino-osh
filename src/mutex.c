#include "os.h"
#include "internal.h"

static void blocked_enq(os_mutex_t *mutex, os_task_t *task) {
    OS_ASSERT(task->nblocked == NULL);
    // osi_printf("%s: queuing for %p existing=%p\n", task->name, mutex, task->mutex);
    if (mutex->blocked.tasks == NULL) {
        mutex->blocked.tasks = task;
    }
    else {
        for (os_task_t *iter = mutex->blocked.tasks; ; iter = iter->nblocked) {
            OS_ASSERT(iter != task);
            if (iter->nblocked == NULL) {
                iter->nblocked = task;
                break;
            }
        }
    }
    OS_ASSERT(task->mutex == NULL);
    task->mutex = mutex;
}

static os_task_t *blocked_deq(os_mutex_t *mutex) {
    os_task_t *task = mutex->blocked.tasks;
    if (task == NULL) {
        return NULL;
    }
    OS_ASSERT(task->mutex == mutex);
    mutex->blocked.tasks = task->nblocked;
    task->mutex = NULL;
    task->nblocked = NULL;
    return task;
}

os_status_t osi_mutex_create(os_mutex_t *mutex) {
    mutex->owner = NULL;
    mutex->blocked.tasks = NULL;
    mutex->level = 0;
    return OSS_SUCCESS;
}

os_status_t osi_mutex_acquire(os_mutex_t *mutex, uint16_t to) {
    os_task_t *task = os_task_self();

    // TODO: Allow tasks to have more than one mutex.
    OS_ASSERT(task->mutex == NULL || task->mutex == mutex);

    // Check for an easy acquire.
    if (mutex->level == 0) {
        // osi_printf("%s: immediate acq\n", task->name);
        mutex->owner = task;
        mutex->level = 1;
        task->mutex = mutex;
        return OSS_SUCCESS;
    }

    if (mutex->owner == os_task_self()) {
        if (mutex->level == (uint32_t)(-1)) {
            return OSS_ERROR;
        }
        mutex->level++;
        return OSS_SUCCESS;
    }

    if (to == 0) {
        return OSS_ERROR_TO;
    }

    // Block until somebody releases.
    blocked_enq(mutex, task);
    svc_block(to, OS_TASK_FLAG_MUTEX);
    return OSS_ERROR_TO;
}

os_status_t osi_mutex_release(os_mutex_t *mutex) {
    OS_ASSERT(mutex->level > 0);
    OS_ASSERT(mutex->owner == os_task_self());

    // See if this is the final release we need.
    if (--mutex->level != 0) {
        return OSS_SUCCESS;
    }

    /* Free the mutex and the check for blocked tasks. */
    OS_ASSERT(mutex->owner->mutex == mutex);
    mutex->owner->mutex = NULL;
    mutex->owner = NULL;

    /* Is somebody waiting for this mutex? */
    if (mutex->blocked.tasks != NULL) {
        os_task_t *blocked_task = blocked_deq(mutex);
        // osi_printf("%s: waking to acquire %p\n", blocked_task->name, mutex);
        mutex->owner = blocked_task;
        mutex->level = 1;
        blocked_task->mutex = mutex;
        osi_task_return_value(blocked_task, OSS_SUCCESS);
        osi_dispatch(blocked_task);
    }

    return OSS_SUCCESS;
}
