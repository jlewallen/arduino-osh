#include "os.h"

os_status_t os_queue_create(os_queue_t *queue, uint16_t size) {
    queue->size = size;
    queue->number = 0;
    queue->first = 0;
    queue->last = 0;
    for (uint16_t i = 0; i < size; ++i) {
        queue->messages[i] = NULL;
    }

    return OSS_SUCCESS;
}

os_status_t os_queue_enqueue(os_queue_t *queue, void *message, uint16_t to) {
    // TODO: Maintain a task queue per mailbox, then we can just send directly.
    if (queue->number == queue->size) {
        if (to == 0) {
            return OSS_ERROR_MEM;
        }
        // We're full, so we need to block until somebody takes one.
        // TODO: This should be a wait with a delay.
        // os_delay(to);
        return OSS_ERROR_TO;
    }

    queue->messages[queue->first] = message;
    queue->number++;
    if (++queue->first == queue->size) {
        queue->first = 0U;
    }

    return OSS_SUCCESS;
}

os_status_t os_queue_dequeue(os_queue_t *queue, void **message, uint16_t to) {
    if (queue->number > 0) {
        *message = queue->messages[queue->last];
        if (++queue->last == queue->size) {
            queue->last = 0;
        }
        queue->number--;
        return OSS_SUCCESS;
    }

    if (to == 0) {
        return OSS_ERROR_MEM;
    }

    // os_delay(to);

    // TODO: Check for a message or the timeout.

    return OSS_ERROR_TO;
}
