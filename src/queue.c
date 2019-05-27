#include "os.h"

inline void blocked_enq(os_queue_t *queue, os_task_t *task) {
    if (queue->blocked == NULL) {
        queue->blocked = task;
    }
    else {
        for (os_task_t *iter = queue->blocked; ; iter = iter->blocked) {
            if (iter->blocked == NULL) {
                iter->blocked = task;
                break;
            }
        }
    }
    task->queue = queue;
}

inline os_task_t *blocked_deq(os_queue_t *queue) {
    os_task_t *task = queue->blocked;
    if (task == NULL) {
        return NULL;
    }
    OSDOTH_ASSERT(task->queue == queue);
    queue->blocked = task->blocked;
    task->queue = NULL;
    task->blocked = NULL;
    return task;
}

os_status_t os_queue_create(os_queue_t *queue, uint16_t size) {
    queue->size = size;
    queue->number = 0;
    queue->first = 0;
    queue->last = 0;
    queue->status = OS_QUEUE_FINE;
    for (uint16_t i = 0; i < size; ++i) {
        queue->messages[i] = NULL;
    }

    return OSS_SUCCESS;
}

os_status_t os_queue_enqueue(os_queue_t *queue, void *message, uint16_t to) {
    os_task_t *running = os_task_self();

    OSDOTH_ASSERT(running != NULL); // TODO: Relax this?
    OSDOTH_ASSERT(running->blocked == NULL);

    /* If there's tasks waiting, we can send directly via their stack. */
    if (queue->blocked != NULL && queue->status == OS_QUEUE_BLOCKED_RECEIVE) {
        os_task_t *blocked_receiver = blocked_deq(queue);

        os_tuple_t *receive_rv = os_task_return_tuple(blocked_receiver);
        receive_rv->status = OSS_SUCCESS;
        receive_rv->value.ptr = message;

        os_dispatch(blocked_receiver);

        return OSS_SUCCESS;
    }

    if (queue->number == queue->size) {
        if (to == 0) {
            return OSS_ERROR_MEM;
        }

        // Store the message we're trying to queue, it'll be queued when space
        // becomes available.
        running->message = message;

        // Block until somebody takes one, freeing space.
        queue->status = OS_QUEUE_BLOCKED_SEND;
        blocked_enq(queue, os_task_self());
        svc_block(to, OS_TASK_FLAG_QUEUE);

        return OSS_ERROR_TO;
    }

    // There's room, so place the message into the end of the queue.
    queue->status = OS_QUEUE_FINE;
    queue->messages[queue->first] = message;
    queue->number++;
    if (++queue->first == queue->size) {
        queue->first = 0U;
    }

    return OSS_SUCCESS;
}

os_status_t os_queue_dequeue(os_queue_t *queue, void **message, uint16_t to) {
    OSDOTH_ASSERT(osg.running != NULL); // TODO: Relax this?
    OSDOTH_ASSERT(osg.running->blocked == NULL);

    if (queue->number > 0) {
        *message = queue->messages[queue->last];
        if (++queue->last == queue->size) {
            queue->last = 0;
        }
        queue->number--;

        // Is somebody waiting to send a message?
        if (queue->blocked != NULL && queue->status == OS_QUEUE_BLOCKED_SEND) {
            os_task_t *blocked_sender = blocked_deq(queue);

            queue->messages[queue->first] = blocked_sender->message;
            if (++queue->first == queue->size) {
                queue->first = 0U;
            }
            queue->number++;
            blocked_sender->message = NULL;

            os_tuple_t *send_rv = os_task_return_tuple(blocked_sender);
            send_rv->status = OSS_SUCCESS;
            send_rv->value.ptr = NULL;

            os_dispatch(blocked_sender);
        }
        return OSS_SUCCESS;
    }

    if (to == 0) {
        return OSS_ERROR_MEM;
    }

    // Block for to ms or until a message comes in.
    queue->status = OS_QUEUE_BLOCKED_RECEIVE;
    blocked_enq(queue, os_task_self());
    svc_block(to, OS_TASK_FLAG_QUEUE);
    return OSS_ERROR_TO;
}
