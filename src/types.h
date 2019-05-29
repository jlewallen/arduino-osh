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
#ifndef OS_TYPES_H
#define OS_TYPES_H

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
/*
#define OS_CONFIG_DEBUG
*/

/**
 *
 */
#define OS_CONFIG_DEBUG_RTT
/*
#define OS_CONFIG_DEBUG_MUTEXES
#define OS_CONFIG_DEBUG_QUEUES
#define OS_CONFIG_DEBUG_SCHEDULE
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
#define OS_STACK_BASIC_FRAME_SIZE                     (16)
#define OS_STACK_EXTENDED_FRAME_SIZE                  (32)
#define OS_STACK_MINIMUM_SIZE_WORDS                   (OS_STACK_EXTENDED_FRAME_SIZE + 8)
#define OS_STACK_MINIMUM_SIZE                         (OS_STACK_MINIMUM_SIZE_WORDS * 4)

/**
 *
 */
#define OS_ASSERT(expression)                         (void)((expression) || (os_assert(#expression, __FILE__, __LINE__), 0))

/**
 *
 */
#define OS_CHECK(expression)                          OS_ASSERT((expression) == OSS_SUCCESS)

/**
 *
 */
typedef enum os_start_status {
    OS_TASK_START_RUNNING,
    OS_TASK_START_SUSPENDED,
} os_start_status;

/**
 *
 */
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

typedef uint32_t os_priority_t;

/**
 *
 */
#define OS_PRIORITY_LOWEST                            (os_priority_t)0x00
#define OS_PRIORITY_IDLE                              (os_priority_t)0x00
#define OS_PRIORITY_NORMAL                            (os_priority_t)0x10
#define OS_PRIORITY_HIGHEST                           (os_priority_t)0xff

/**
 *
 */
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
    struct os_task_t *nrp;
    struct os_task_t *nblocked;
    struct os_queue_t *queue;
    struct os_mutex_t *mutex;
    os_priority_t priority;
    void *message;
    uint32_t started;
    uint32_t delay;
    uint32_t flags;
    #if defined(OS_CONFIG_DEBUG)
    uint32_t debug_stack_max;
    #endif
} os_task_t;

/**
 *
 */
typedef enum os_queue_status_t {
    OS_QUEUE_FINE,
    OS_QUEUE_BLOCKED_SEND,
    OS_QUEUE_BLOCKED_RECEIVE,
} os_queue_status_t;

/**
 *
 */
typedef struct os_blocked_t {
    uint32_t type;
    os_task_t *tasks;
} os_blocked_t;

/**
 *
 */
typedef struct os_queue_definition_t {
    const char *name;
    uint16_t size;
} os_queue_definition_t;

/**
 *
 */
typedef struct os_queue_t {
    os_queue_definition_t *def;
    os_blocked_t blocked;
    uint16_t size;
    uint16_t number;
    uint16_t first;
    uint16_t last;
    os_queue_status_t status;
    void *messages[1];
} os_queue_t;

/**
 *
 */
typedef struct os_mutex_definition_t {
    const char *name;
} os_mutex_definition_t;


/**
 *
 */
typedef struct os_mutex_t {
    os_mutex_definition_t *def;
    os_blocked_t blocked;
    os_task_t *owner;
    uint16_t level;
} os_mutex_t;

/**
 *
 */
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
    os_task_t *idle;                 //! The idle task. */
    os_task_t *tasks;                //! Immutable, every task in order of creation. */
    os_task_t *queue;                //! Queue of tasks waiting for a turn to run. */
    os_task_t *waiting;              //! Tasks waiting for something. */
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
 *
 */
typedef struct os_basic_sframe_t {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    void *lr;
    void *pc;
    uint32_t xpsr;
} os_basic_sframe_t;

/**
 *
 */
typedef struct os_our_sframe_t {
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    os_basic_sframe_t basic;
} os_our_sframe_t;

/**
 *
 */
typedef struct os_extended_sframe_t {
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    os_basic_sframe_t basic;
    uint32_t s0, s1, s2, s3, s4;
    uint32_t s5, s6, s7, s8, s9;
    uint32_t s10, s11, s12, s13;
    uint32_t s14, s15, fpscr, reserved0;
} os_extended_sframe_t;

#if defined(__cplusplus)
}
#endif

#endif // OS_TYPES_H
