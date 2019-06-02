/**
 *
 */
#include "os.h"
#include "internal.h"
#include "syscalls.h"

uint32_t svc_example() {
    osi_printf("svc_example\n");
    return 0;
}

uint32_t svc_delay(uint32_t ms) {
    OS_ASSERT(osg.running != NULL);
    OS_ASSERT(osg.scheduled != osg.running);

    osg.running->status = OS_TASK_STATUS_WAIT;
    osg.running->delay = os_uptime() + ms;

    if (osg.scheduled == NULL) {
        osi_schedule();
    }

    OS_ASSERT(osg.scheduled != NULL);
    OS_ASSERT(osg.scheduled != osg.running);

    return OSS_ERROR_TO;
}

uint32_t svc_block(uint32_t ms, uint32_t flags) {
    osg.running->flags |= flags;
    uint32_t status = svc_delay(ms);

    return status;
}

uint32_t os_printf(const char *str, ...) {
    return 0;
}

uint32_t svc_pstr(const char *str) {
    return osi_printf(str);
}

os_status_t svc_queue_create(os_queue_t *queue, os_queue_definition_t *def) {
    return osi_queue_create(queue, def);
}

os_tuple_return_type_t svc_queue_enqueue(os_queue_t *queue, void *message, uint32_t to) {
    os_tuple_t rtuple = { OSS_ERROR_TO, { 0 } };

    rtuple.status = osi_queue_enqueue(queue, message, (uint16_t)to);

    return os_tuple_return_value(rtuple);
}

os_tuple_return_type_t svc_queue_dequeue(os_queue_t *queue, uint32_t to) {
    os_tuple_t rtuple = { OSS_ERROR_TO, { 0 } };

    rtuple.status = osi_queue_dequeue(queue, &rtuple.value.ptr, (uint16_t)to);

    return os_tuple_return_value(rtuple);
}

os_status_t svc_mutex_create(os_mutex_t *mutex, os_mutex_definition_t *def) {
    return osi_mutex_create(mutex, def);
}

os_status_t svc_mutex_acquire(os_mutex_t *mutex, uint32_t to) {
    return osi_mutex_acquire(mutex, (uint16_t)to);
}

os_status_t svc_mutex_release(os_mutex_t *mutex) {
    return osi_mutex_release(mutex);
}

/**
 * Application facing service call wrappers.
 */

#if !(defined(__SAMD21__) || defined(__SAMD51__))
#define __get_CONTROL()  0x0
#define __get_IPSR()     0
#endif

/**
 * Returns true if the current execution context is privileged.
 */
inline static bool osi_is_privileged() {
    return (__get_CONTROL() & 0x1) == 0x0;
}

/**
 * Returns true if the current execution context is using PSP.
 */
inline static bool osi_in_task() {
    return (__get_CONTROL() & 0x2) == 0x2;
}

uint32_t os_example() {
    if (osi_in_task()) {
        return __svc_example();
    }
    else {
        return svc_example();
    }
}

uint32_t os_delay(uint32_t ms) {
    if (osi_in_task()) {
        return __svc_delay(ms);
    }
    else {
        return OSS_ERROR_INVALID;
    }
}

uint32_t os_block(uint32_t ms, uint32_t flags) {
    if (osi_in_task()) {
        return __svc_block(ms, flags);
    }
    else {
        return OSS_ERROR_INVALID;
    }
}

uint32_t os_pstr(const char *str) {
    if (__get_IPSR() != 0U) {
        return OSS_ERROR_INVALID;
    }
    if (osi_in_task()) {
        return __svc_pstr(str);
    }
    else {
        return svc_pstr(str);
    }
}

os_status_t os_queue_create(os_queue_t *queue, os_queue_definition_t *def) {
    if (__get_IPSR() != 0U) {
        return OSS_ERROR_INVALID;
    }
    if (osi_in_task()) {
        return __svc_queue_create(queue, def);
    }
    return svc_queue_create(queue, def);
}

os_tuple_t os_queue_enqueue(os_queue_t *queue, void *message, uint32_t to) {
    return __svc_queue_enqueue(queue, message, to);
}

os_tuple_t os_queue_dequeue(os_queue_t *queue, uint32_t to) {
    return __svc_queue_dequeue(queue, to);
}

os_status_t os_mutex_create(os_mutex_t *mutex, os_mutex_definition_t *def) {
    if (__get_IPSR() != 0U) {
        return OSS_ERROR_INVALID;
    }
    if (osi_in_task()) {
        return __svc_mutex_create(mutex, def);
    }
    return svc_mutex_create(mutex, def);
}

os_status_t os_mutex_acquire(os_mutex_t *mutex, uint32_t to) {
    return __svc_mutex_acquire(mutex, to);
}

os_status_t os_mutex_release(os_mutex_t *mutex) {
    return __svc_mutex_release(mutex);
}
