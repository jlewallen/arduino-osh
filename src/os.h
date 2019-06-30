/**
 *
 * This file used to be a part of os.h, and has significantly evolved.
 *
 * Huge thanks to Adam Heinrich <adam@adamh.cz> for the original inspiration.
 * See the README. for more information.
 *
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
#ifndef OS_H
#define OS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#if defined(ARDUINO)
#include <sam.h>
#else
#endif

#include "printf.h"
#include "segger/SEGGER_RTT.h"
#include "types.h"
#include "arduino.h"
#include "linux.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Map an os_status_t to a string for display/logging.
 */
inline const char *os_status_str(os_status_t status) {
    switch (status) {
    case OSS_SUCCESS: return "OSS_SUCCESS";
    case OSS_ERROR_TO: return "OSS_ERROR_TO";
    case OSS_ERROR_MEM: return "OSS_ERROR_MEM";
    case OSS_ERROR_INT: return "OSS_ERROR_INT";
    case OSS_ERROR_INVALID: return "OSS_ERROR_INVALID";
    default: return "UNKNOWN";
    }
}

/**
 * Map an os_task_status_t to a string for display/logging.
 */
inline const char *os_task_status_str(os_task_status status) {
    switch (status) {
    case OS_TASK_STATUS_IDLE: return "OS_TASK_STATUS_IDLE";
    case OS_TASK_STATUS_ACTIVE: return "OS_TASK_STATUS_ACTIVE";
    case OS_TASK_STATUS_WAIT: return "OS_TASK_STATUS_WAIT";
    case OS_TASK_STATUS_SUSPENDED: return "OS_TASK_STATUS_SUSPENDED";
    case OS_TASK_STATUS_FINISHED: return "OS_TASK_STATUS_FINISHED";
    default: return "UNKNOWN";
    }
}

/**
 * Return the currently executing task.
 */
#define os_task_self()    ((os_task_t *)osg.running)
/*
inline os_task_t *os_task_self() {
    return (os_task_t *)osg.running;
}
*/

/**
 * Return the name of the currently executing task.
 */
inline const char *os_task_name() {
    return osg.running->name;
}

/**
 *
 */
os_status_t os_initialize();

/**
 *
 */
os_status_t os_task_initialize(os_task_t *task, const char *name,
                               os_start_status status,
                               void (*handler)(void *params), void *params,
                               uint32_t *stack, size_t stack_size);

/**
 *
 */
os_status_t os_start();

/**
 *
 */
os_status_t os_teardown();

/**
 *
 */
os_task_status os_task_get_status(os_task_t *task);

/**
 *
 */
os_status_t os_task_start(os_task_t *task);

/**
 *
 */
os_status_t os_task_suspend(os_task_t *task);

/**
 *
 */
os_status_t os_task_resume(os_task_t *task);

/**
 *
 */
uint32_t os_task_uptime(os_task_t *task);

/**
 *
 */
uint32_t os_task_runtime(os_task_t *task);

/**
 *
 */
uint32_t os_uptime();

/**
 *
 */
uint32_t osi_printf(const char *f, ...);

/**
 *
 */
uint32_t osi_vprintf(const char *f, va_list args);

/**
 *
 */
uint32_t os_free_memory();

/**
 *
 */
void osi_assert(const char *assertion, const char *file, int line);

#if defined(__cplusplus)
}
#endif

#include "service.h"

#endif /* OS_H */
