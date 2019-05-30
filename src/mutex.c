#include "os.h"
#include "internal.h"

static void blocked_enq(os_mutex_t *mutex, os_task_t *task) {
    blocked_append(&mutex->blocked, task);
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
    task->nblocked = NULL;
    return task;
}

os_status_t osi_mutex_create(os_mutex_t *mutex, os_mutex_definition_t *def) {
    mutex->def = def;
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
        mutex->owner = task;
        mutex->level = 1;
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
    OS_ASSERT(mutex->owner->mutex == NULL);
    mutex->owner = NULL;

    /* Is somebody waiting for this mutex? */
    if (mutex->blocked.tasks != NULL) {
        os_task_t *blocked_task = blocked_deq(mutex);
        mutex->owner = blocked_task;
        mutex->level = 1;
        osi_task_set_stacked_return(blocked_task, OSS_SUCCESS);
        osi_dispatch(blocked_task);
    }

    return OSS_SUCCESS;
}
