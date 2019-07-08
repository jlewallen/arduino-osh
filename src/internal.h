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
#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "segger/SEGGER_RTT.h"
#include "mutex.h"
#include "queue.h"
#include "arduino.h"
#include "syscalls.h"
#include "faults.h"
#include "utilities.h"
#include "platform.h"

#if defined(ARDUINO)
#define OS_NORETURN  __attribute__ ((noreturn))
#else
#define OS_NORETURN
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
os_status_t osi_dispatch(os_task_t *task);

/**
 *
 */
os_status_t osi_schedule();

/**
 *
 */
os_status_t osi_irs_systick();

/**
 *
 */
void osi_stack_check();

/**
 *
 */
uint32_t *osi_task_return_regs(os_task_t *task);

/**
 *
 */
os_tuple_t *osi_task_stacked_return_tuple(os_task_t *task);

/**
 *
 */
uint32_t osi_task_get_stacked_return(os_task_t *task);

/**
 *
 */
uint32_t osi_task_set_stacked_return(os_task_t *task, uint32_t v0);

typedef enum {
    OS_ERROR_NONE,
    OS_ERROR_ASSERTION,
    OS_ERROR_STACK_OVERFLOW,
    OS_ERROR_APP,
} os_error_kind_t;

/**
 *
 */
void osi_error(os_error_kind_t code) OS_NORETURN;

#if defined(__cplusplus)
}
#endif

#endif // OS_INTERNAL_H
