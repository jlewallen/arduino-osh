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
#include "printf.h"
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
/*
#define OSDOTH_CONFIG_DEBUG_MUTEXES
#define OSDOTH_CONFIG_DEBUG_QUEUES
*/

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

#define OS_TASK_FLAG_NONE                             (0)
#define OS_TASK_FLAG_MUTEX                            (1)
#define OS_TASK_FLAG_QUEUE                            (2)

struct os_queue_t;
struct os_mutex_t;

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
    struct os_task_t *blocked;
    struct os_queue_t *queue;
    struct os_mutex_t *mutex;
    void *message;
    uint32_t started;
    uint32_t delay;
    uint32_t flags;

    #if defined(OSDOTH_CONFIG_DEBUG)
    uint32_t debug_stack_max;
    #endif
} os_task_t;

typedef enum os_queue_status_t {
    OS_QUEUE_FINE,
    OS_QUEUE_BLOCKED_SEND,
    OS_QUEUE_BLOCKED_RECEIVE,
} os_queue_status_t;

typedef struct os_queue_t {
    os_task_t *blocked;
    uint16_t size;
    uint16_t number;
    uint16_t first;
    uint16_t last;
    os_queue_status_t status;
    void *messages[1];
} os_queue_t;

typedef struct os_mutex_t {
    os_task_t *owner;
    os_task_t *blocked;
    uint16_t level;
} os_mutex_t;

typedef enum {
    OS_STATE_DEFAULT = 1,
    OS_STATE_INITIALIZED,
    OS_STATE_TASKS_INITIALIZED,
    OS_STATE_STARTED,
} os_state_t;

/**
 * Struct with global operating system state.
 */
typedef struct os_globals_t {
    volatile os_task_t *running;
    volatile os_task_t *scheduled;
    os_state_t state;
    uint8_t ntasks;
    os_task_t *idle;
    os_task_t *tasks;
    os_task_t *delayed;
} os_globals_t;

/**
 * Singleton instance of state for the operating system.
 */
extern os_globals_t osg;

/* TODO: typedef enum here breaks in the service call macro magic. */
typedef uint32_t os_status_t;

#define OSS_SUCCESS                                   (0x0) /** Successful call. */
#define OSS_ERROR                                     (0x1) /** A generic error. */
#define OSS_ERROR_TO                                  (0x2) /** Timeout related rror. */
#define OSS_ERROR_MEM                                 (0x3) /** Insufficient memory. */
#define OSS_ERROR_INT                                 (0x4) /** Operation was interrupted. */
#define OSS_ERROR_INVALID                             (0x5) /** Invalid operation. */

/**
 * Map an os_status_t to a string for display/logging.
 */
inline const char *os_status_str(os_status_t status) {
    switch (status) {
    case OSS_SUCCESS: return "OSS_SUCCESS";
    case OSS_ERROR_TO: return "OSS_ERROR_TO";
    case OSS_ERROR_MEM: return "OSS_ERROR_MEM";
    case OSS_ERROR_INT: return "OSS_ERROR_INT";
    default: return "UNKNOWN";
    }
}

/**
 * Tuple for returning multiple values from a service call. This is modified in
 * place on the stack by other tasks to fixup return values for blocked operations.
 */
typedef struct {
    os_status_t status;
    union  {
        uint32_t u32;
        void *ptr;
    } value;
} os_tuple_t;

/**
 * Return the currently executing task.
 */
inline os_task_t *os_task_self() {
    return (os_task_t *)osg.running;
}

/**
 * Return the name of the currently executing task.
 */
inline const char *os_task_name() {
    return osg.running->name;
}

/**
 *
 */
bool os_initialize();

/**
 *
 */
bool os_task_initialize(os_task_t *task, const char *name,
                        os_start_status status,
                        void (*handler)(void *params), void *params,
                        uint32_t *stack, size_t stack_size);

/**
 *
 */
bool os_start();

/**
 *
 */
os_task_status os_task_get_status(os_task_t *task);

/**
 *
 */
bool os_task_start(os_task_t *task);

/**
 *
 */
bool os_task_suspend(os_task_t *task);

/**
 *
 */
bool os_task_resume(os_task_t *task);

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
uint32_t os_printf(const char *f, ...);

/**
 *
 */
uint32_t os_free_memory();

/**
 *
 */
void os_assert(const char *assertion, const char *file, int line) __attribute__ ((noreturn));

#if defined(__cplusplus)
}
#endif

#include "service.h"

#endif /* OS_H */
