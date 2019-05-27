#include "os.h"

uint32_t os_svc_example() {
    os_printf("os_svc_example\n");
    return 0;
}

uint32_t os_svc_delay(uint32_t ms) {
    OSDOTH_ASSERT(osg.running != NULL);
    OSDOTH_ASSERT(osg.scheduled != osg.running);

    osg.running->status = OS_TASK_STATUS_WAIT;
    osg.running->delay = os_uptime() + ms;

    if (osg.scheduled == NULL) {
        os_schedule();
    }

    OSDOTH_ASSERT(osg.scheduled != NULL);
    OSDOTH_ASSERT(osg.scheduled != osg.running);

    return 0;
}

uint32_t os_svc_printf(const char *str) {
    return os_printf(str);
}

os_status_t svc_queue_create(os_queue_t *queue, uint32_t size) {
    return os_queue_create(queue, size);
}

os_status_t svc_queue_enqueue(os_queue_t *queue, void *message, uint32_t to) {
    return os_queue_enqueue(queue, message, (uint16_t)to);
}

os_status_t svc_queue_dequeue(os_queue_t *queue, void **message, uint32_t to) {
    return os_queue_dequeue(queue, message, (uint16_t)to);
}
