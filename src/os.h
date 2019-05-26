/*
 * This file is part of os.h.
 *
 * Copyright (C) 2016 Adam Heinrich <adam@adamh.cz>
 *
 * Os.h is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Os.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with os.h.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OS_H
#define OS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sam.h>

#include "arduino.h"
#include "service.h"
#include "segger/SEGGER_RTT.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
/*
#define OSDOTH_CONFIG_DEBUG
*/

/**
 *
 */
#define OSDOTH_CONFIG_DEBUG_RTT

/**
 *
 */
/*
#define DEBUG_MTB
*/

#define DEBUG_MTB_MAGNITUDE_PACKETS                   (7)
#define DEBUG_MTB_SIZE                                (1 << (DEBUG_MTB_MAGNITUDE_PACKETS + 1))
#define DEBUG_MTB_SIZE_BYTES                          (DEBUG_MTB_SIZE << 3)

/**
 * Minimum number of bytes for a stack.
 */
#define OSDOTH_STACK_BASIC_FRAME_SIZE                 (16)
#define OSDOTH_STACK_EXTENDED_FRAME_SIZE              (32)
#define OSDOTH_STACK_MINIMUM_SIZE_WORDS               (OSDOTH_STACK_EXTENDED_FRAME_SIZE + 8)
#define OSDOTH_STACK_MINIMUM_SIZE                     (OSDOTH_STACK_MINIMUM_SIZE_WORDS * 4)

/**
 *
 */
#define OSDOTH_ASSERT(expression)                     (void)((expression) || (os_assert(#expression, __FILE__, __LINE__), 0))

typedef enum os_start_status {
    OS_TASK_START_RUNNING,
    OS_TASK_START_SUSPENDED,
} os_start_status;

typedef enum os_task_status {
    OS_TASK_STATUS_IDLE = 1,
    OS_TASK_STATUS_ACTIVE,
    OS_TASK_STATUS_WAIT,
    OS_TASK_STATUS_SUSPENDED,
    OS_TASK_STATUS_FINISHED
} os_task_status;

typedef struct os_task_t {
    /* The stack pointer (sp) has to be the first element as it is located
       at the same address as the structure itself (which makes it possible
       to locate it safely from assembly implementation of PendSV_Handler).
       The compiler might add padding between other structure elements. */
    volatile void *sp;
    volatile void *stack;
    volatile uint32_t stack_size;
    volatile uint8_t stack_kind;
    volatile enum os_task_status status;
    const char *name;
    void (*handler)(void*);
    void *params;
    struct os_task_t *np;
    struct os_task_t *delayed;
    uint32_t started;
    uint32_t delay;
    #if defined(OSDOTH_CONFIG_DEBUG)
    uint32_t debug_stack_max;
    #endif
} os_task_t;

typedef enum {
    OS_STATE_DEFAULT = 1,
    OS_STATE_INITIALIZED,
    OS_STATE_TASKS_INITIALIZED,
    OS_STATE_STARTED,
} os_state;

typedef enum {
    OS_ERROR_NONE,
    OS_ERROR_ASSERTION,
    OS_ERROR_STACK_OVERFLOW,
    OS_ERROR_APP,
} os_error_kind;

typedef struct os_globals_t {
    volatile os_task_t *running;
    volatile os_task_t *scheduled;
    os_state state;
    uint8_t ntasks;
    os_task_t *idle;
    os_task_t *tasks;
    os_task_t *delayed;
} os_globals_t;

extern os_globals_t osg;

bool os_platform_setup();

bool os_initialize();

bool os_task_initialize(os_task_t *task, const char *name,
                        os_start_status status,
                        void (*handler)(void *params), void *params,
                        uint32_t *stack, size_t stack_size);

bool os_task_start(os_task_t *task);

bool os_task_suspend(os_task_t *task);

bool os_task_resume(os_task_t *task);

os_task_status os_task_get_status(os_task_t *task);

uint32_t os_task_stack_usage(os_task_t *task);

bool os_self_suspend();

bool os_self_resume();

bool os_start();

void os_schedule();

void os_irs_systick();

uint32_t os_uptime();

const char *os_task_name();

uint32_t os_task_uptime();

uint32_t os_task_runtime();

void os_delay(uint32_t ms);

void os_assert(const char *assertion, const char *file, int line);

void os_error(uint8_t code);

void os_yield();

void os_stack_check();

int32_t os_printf(const char *f, ...);

uint32_t os_free_memory();

#if defined(__cplusplus)
}
#endif

#endif /* OS_H */
