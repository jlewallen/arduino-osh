/**
 * This software is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this source code. If not, see <http://www.gnu.org/licenses/>.
 */
#include "os.h"
#include "internal.h"
#include "syscalls.h"
#include "platform.h"

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
    return svc_delay(ms);
}

uint32_t svc_printf(const char *f, void *vargs) {
    va_list *args = (va_list *)vargs;
    return osi_vprintf(f, *args);
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
inline static bool osi_in_idle() {
    return os_task_self()->priority == OS_PRIORITY_IDLE;
}

uint32_t os_delay(uint32_t ms) {
    if (osi_in_task()) {
        if (osi_in_idle()) {
            osi_platform_delay(ms);
            return ms;
        }
        else {
            return __svc_delay(ms);
        }
    }
    return osi_platform_delay(ms);
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

uint32_t os_printf(const char *f, ...) {
    uint32_t rval;
    va_list args;
    va_start(args, f);
    if (osi_in_task()) {
        rval = __svc_printf(f, &args);
    }
    else {
        rval = svc_printf(f, &args);
    }
    va_end(args);
    return rval;
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
    if (__get_IPSR() != 0U) {
        OS_ASSERT(to == 0);
        return osi_queue_enqueue_isr(queue, message);
    }
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
