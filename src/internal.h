/**
 *
 */
#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "segger/SEGGER_RTT.h"
#include "mutex.h"
#include "queue.h"
#include "arduino.h"
#include "syscalls.h"
#include "faults.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
bool osi_platform_setup();

/**
 *
 */
void osi_dispatch(os_task_t *task);

/**
 *
 */
void osi_schedule();

/**
 *
 */
void osi_irs_systick();

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
os_tuple_t *osi_task_return_tuple(os_task_t *task);

/**
 *
 */
void osi_task_return_value(os_task_t *task, uint32_t v0);

typedef enum {
    OS_ERROR_NONE,
    OS_ERROR_ASSERTION,
    OS_ERROR_STACK_OVERFLOW,
    OS_ERROR_APP,
} os_error_kind_t;

/**
 *
 */
void osi_error(os_error_kind_t code) __attribute__ ((noreturn));

#if defined(__cplusplus)
}
#endif

#endif // OS_INTERNAL_H
