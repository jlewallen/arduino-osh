#include "os.h"

uint32_t svc_example() {
    os_printf("svc_example\n");
    return 0;
}

uint32_t svc_delay(uint32_t ms) {
    OSDOTH_ASSERT(osg.running != NULL);
    OSDOTH_ASSERT(osg.scheduled != osg.running);

    osg.running->status = OS_TASK_STATUS_WAIT;
    osg.running->delay = os_uptime() + ms;

    if (osg.scheduled == NULL) {
        os_schedule();
    }

    uint32_t status = OSS_ERROR_TO;

    OSDOTH_ASSERT(osg.scheduled != NULL);
    OSDOTH_ASSERT(osg.scheduled != osg.running);

    return status;
}

uint32_t svc_block(uint32_t ms, uint32_t flags) {
    osg.running->flags |= flags;
    uint32_t status = svc_delay(ms);

    return status;
}

uint32_t svc_printf(const char *str) {
    return os_printf(str);
}

os_status_t svc_queue_create(os_queue_t *queue, uint32_t size) {
    return os_queue_create(queue, size);
}

os_tuple_return_type_t svc_queue_enqueue(os_queue_t *queue, void *message, uint32_t to) {
    os_tuple_t rtuple = { OSS_ERROR_TO, { 0 } };

    rtuple.status = os_queue_enqueue(queue, message, (uint16_t)to);

    return os_tuple_return_value(rtuple);
}

os_tuple_return_type_t svc_queue_dequeue(os_queue_t *queue, uint32_t to) {
    os_tuple_t rtuple = { OSS_ERROR_TO, { 0 } };

    rtuple.status = os_queue_dequeue(queue, &rtuple.value.ptr, (uint16_t)to);

    return os_tuple_return_value(rtuple);
}


os_status_t svc_mutex_create(os_mutex_t *mutex) {
    return os_mutex_create(mutex);
}

os_status_t svc_mutex_acquire(os_mutex_t *mutex, uint32_t to) {
    return os_mutex_acquire(mutex, (uint16_t)to);
}

os_status_t svc_mutex_release(os_mutex_t *mutex) {
    return os_mutex_release(mutex);
}
