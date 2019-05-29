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

//#include <inttypes.h>
// #include <sam.h>

#include "os.h"
#include "internal.h"

os_globals_t osg = {
    NULL,  /* running */
    NULL,  /* scheduled */
    OS_STATE_DEFAULT,
    0,     /* ntasks */
    NULL,  /* idle */
    NULL,  /* tasks */
    NULL,  /* delayed */
};

#if defined(OSH_MTB)
__attribute__((__aligned__(DEBUG_MTB_SIZE_BYTES))) uint32_t mtb[DEBUG_MTB_SIZE];
#endif

#define OSH_STACK_MAGIC_WORD     0xE25A2EA5U
#define OSH_STACK_MAGIC_PATTERN  0xCCCCCCCCU

static void infinite_loop() __attribute__ ((noreturn));

static void task_finished() __attribute__ ((noreturn));

static uint32_t runqueue_length(os_task_t *head);

static void runqueue_add(os_task_t **head, os_task_t *task);

static void runqueue_remove(os_task_t **head, os_task_t *task);

static void waitqueue_add(os_task_t **head, os_task_t *task);

static void waitqueue_remove(os_task_t **head, os_task_t *task);

os_status_t os_initialize() {
    if (osg.state != OS_STATE_DEFAULT) {
        return OSS_ERROR_INVALID;
    }

    #if defined(OSH_MTB)
    REG_MTB_POSITION = ((uint32_t)(mtb - REG_MTB_BASE)) & 0xFFFFFFF8;
    REG_MTB_FLOW = (((uint32_t)mtb - REG_MTB_BASE) + DEBUG_MTB_SIZE_BYTES) & 0xFFFFFFF8;
    REG_MTB_MASTER = 0x80000000 + (DEBUG_MTB_MAGNITUDE_PACKETS - 1);
    #endif

    osg.state = OS_STATE_INITIALIZED;

    return OSS_SUCCESS;
}

uint32_t *initialize_stack(os_task_t *task, uint32_t *stack, size_t stack_size) {
    OS_ASSERT((stack_size % sizeof(uint32_t)) == 0);
    OS_ASSERT(stack_size >= OS_STACK_MINIMUM_SIZE);

    uint32_t stack_offset = (stack_size / sizeof(uint32_t));
    for (uint32_t i = 0; i < stack_offset; ++i) {
        stack[i] = OSH_STACK_MAGIC_PATTERN;
    }

    // Check alignment. May not be necessary?
    uint32_t *stk = stack + stack_offset;
    if ((uint32_t)stk & 0x04U) {
        stk--;
    }
    stk -= OS_STACK_BASIC_FRAME_SIZE;

    /* Save values of registers which will be restored on exc. return:
       - XPSR: Default value (0x01000000)
       - PC: Point to the handler function
       - LR: Point to a function to be called when the handler returns
       - R0: Point to the handler function's parameter */
    stk[15] = 0x01000000;
    stk[14] = (uint32_t)task->handler & ~0x01UL;
    stk[13] = (uint32_t)&task_finished;
    stk[ 8] = (uint32_t)task->params;
    #if defined(OS_CONFIG_DEBUG)
    uint32_t base = 1000 * (osg.ntasks + 1);
    stk[12] = base + 12; /* R12 */
    stk[11] = base + 3;  /* R3  */
    stk[10] = base + 2;  /* R2  */
    stk[ 9] = base + 1;  /* R1  */
    /* stk[ 8] is R0 */
    stk[ 7] = base + 7;  /* R7  */
    stk[ 6] = base + 6;  /* R6  */
    stk[ 5] = base + 5;  /* R5  */
    stk[ 4] = base + 4;  /* R4  */
    stk[ 3] = base + 11; /* R11 */
    stk[ 2] = base + 10; /* R10 */
    stk[ 1] = base + 9;  /* R9  */
    stk[ 0] = base + 8;  /* R8  */
    #endif /* OS_CONFIG_DEBUG */

    // Magic word to check for overflows.
    stack[0] = OSH_STACK_MAGIC_WORD;

    return stk;
}

os_status_t os_task_initialize(os_task_t *task, const char *name, os_start_status status, void (*handler)(void *params), void *params, uint32_t *stack, size_t stack_size) {
    if (osg.state != OS_STATE_INITIALIZED && osg.state != OS_STATE_TASKS_INITIALIZED) {
        return OSS_ERROR_INVALID;
    }

    task->sp = NULL; /* This gets set correctly later. */
    task->stack = stack;
    task->stack_size = stack_size;
    task->stack_kind = 0;
    task->params = params;
    task->handler = handler;
    task->status = status == OS_TASK_START_RUNNING ? OS_TASK_STATUS_IDLE : OS_TASK_STATUS_SUSPENDED;
    task->name = name;
    task->delay = 0;
    task->flags = 0;
    task->started = os_uptime();
    task->queue = NULL;
    task->mutex = NULL;
    task->nblocked = NULL;
    task->nrp = NULL;
    task->priority = OS_PRIORITY_NORMAL;
    #if defined(OS_CONFIG_DEBUG)
    task->debug_stack_max = 0;
    #endif

    task->sp = initialize_stack(task, stack, stack_size);

    task->np = osg.tasks;
    osg.tasks = task;
    osg.ntasks++;

    /* First task initialized is always the idle task, it's also the task that
     * gets a turn first. */
    if (osg.idle == NULL) {
        OS_ASSERT(task->status != OS_TASK_STATUS_SUSPENDED);
        task->priority = OS_PRIORITY_IDLE;
        osg.idle = task;
    }

    /* If the task is ready to go, add to the runqueue. */
    if (task->status == OS_TASK_STATUS_IDLE) {
        runqueue_add(&osg.queue, task);
    }

    osg.state = OS_STATE_TASKS_INITIALIZED;

    return OSS_SUCCESS;
}

os_status_t os_task_start(os_task_t *task) {
    OS_ASSERT(task != NULL);
    OS_ASSERT(task->status != OS_TASK_STATUS_IDLE && task->status != OS_TASK_STATUS_ACTIVE);

    task->stack_kind = 0;
    task->delay = 0;
    task->flags = 0;
    task->queue = NULL;
    task->mutex = NULL;
    task->nblocked = NULL;
    task->started = os_uptime();
    #if defined(OS_CONFIG_DEBUG)
    task->debug_stack_max = 0;
    #endif
    task->sp = initialize_stack(task, (uint32_t *)task->stack, task->stack_size);
    task->status = OS_TASK_STATUS_IDLE;

    // Kind of a hack :)
    waitqueue_remove(&osg.waiting, task);
    waitqueue_remove(&osg.queue, task);
    waitqueue_add(&osg.queue, task);

    #if defined(OS_CONFIG_DEBUG_SCHEDULE)
    os_printf("%s: started\n", task->name);
    #endif

    // Avoid a warning.
    runqueue_length(osg.queue);

    return OSS_SUCCESS;
}

os_status_t os_task_suspend(os_task_t *task) {
    OS_ASSERT(task != NULL);
    OS_ASSERT(task->status == OS_TASK_STATUS_IDLE || task->status == OS_TASK_STATUS_ACTIVE);

    #if defined(OS_CONFIG_DEBUG_SCHEDULE)
    os_printf("%s: suspended\n", task->name);
    #endif

    task->status = OS_TASK_STATUS_SUSPENDED;

    waitqueue_remove(&osg.queue, task);
    runqueue_remove(&osg.queue, task);

    return OSS_SUCCESS;
}

os_status_t os_task_resume(os_task_t *task) {
    OS_ASSERT(task != NULL);
    OS_ASSERT(task->status == OS_TASK_STATUS_SUSPENDED);

    #if defined(OS_CONFIG_DEBUG_SCHEDULE)
    os_printf("%s: resumed\n", task->name);
    #endif

    task->status = OS_TASK_STATUS_IDLE;

    runqueue_add(&osg.queue, task);

    return OSS_SUCCESS;
}

uint32_t os_task_uptime(os_task_t *task) {
    OS_ASSERT(task != NULL);
    return os_uptime() - task->started;
}

uint32_t os_task_runtime(os_task_t *task) {
    OS_ASSERT(task != NULL);
    return os_uptime() - task->started;
}

os_task_status os_task_get_status(os_task_t *task) {
    return task->status;
}

os_status_t os_start(void) {
    if (osg.state != OS_STATE_TASKS_INITIALIZED) {
        return OSS_ERROR_INVALID;
    }

    OS_ASSERT(osi_platform_setup() == OSS_SUCCESS);
    OS_ASSERT(osg.queue != NULL);

    NVIC_SetPriority(PendSV_IRQn, 0xff);
    NVIC_SetPriority(SysTick_IRQn, 0x00);

    /* Running task is the first task in the runqueue. */
    osg.running = osg.queue;

    /* Set PSP to the top of task's stack */
    __set_PSP((uint32_t)osg.running->sp + OS_STACK_BASIC_FRAME_SIZE);
    /* Switch to Unprivilleged Thread Mode with PSP */
    __set_CONTROL(0x03);
    /* Execute DSB/ISB after changing CONTORL (recommended) */
    __DSB();
    __ISB();

    osg.state = OS_STATE_STARTED;
    osg.running->handler(osg.running->params);

    OS_ASSERT(0);

    return true;
}

os_status_t osi_dispatch(os_task_t *task) {
    OS_ASSERT(task != NULL);
    OS_ASSERT(osg.running != NULL);
    // OS_ASSERT(task != osg.running);

    #if defined(OS_CONFIG_DEBUG_SCHEDULE)
    if (task != osg.running) {
        os_printf("%s: d\n", task->name);
    }
    #endif

    if ((task->flags & OS_TASK_FLAG_MUTEX) == OS_TASK_FLAG_MUTEX) {
        OS_ASSERT(task->queue == NULL);
        OS_ASSERT(task->mutex != NULL);
        // OS_ASSERT(task->mutex->blocked.tasks == task);

        #if defined(OS_CONFIG_DEBUG_MUTEXES)
        os_printf("%s: removed from mutex %p\n", task->name, task->mutex);
        #endif

        task->mutex->blocked.tasks = task->mutex->blocked.tasks->nblocked;
        task->nblocked = NULL;
        task->queue = NULL;
        task->mutex = NULL;
        task->flags = 0;
    }
    if ((task->flags & OS_TASK_FLAG_QUEUE) == OS_TASK_FLAG_QUEUE) {
        OS_ASSERT(task->mutex == NULL);
        OS_ASSERT(task->queue != NULL);

        #if defined(OS_CONFIG_DEBUG_QUEUES)
        os_printf("%s: removed from queue %p\n", task->name, task->queue);
        #endif

        task->queue->blocked.tasks = task->queue->blocked.tasks->nblocked;
        task->nblocked = NULL;
        task->queue = NULL;
        task->mutex = NULL;
        task->flags = 0;
    }

    // If this task was waiting and is being given a chance, change queues.
    if (task->status == OS_TASK_STATUS_WAIT) {
        waitqueue_remove(&osg.waiting, task);
        runqueue_add(&osg.queue, task);
        #if defined(OS_CONFIG_DEBUG_SCHEDULE)
        os_printf("%s: running\n", task->name);
        #endif
    }

    task->delay = 0;
    task->flags = 0;
    task->status = OS_TASK_STATUS_ACTIVE;

    // NOTE: Should the status update happen in the PendSV?
    os_task_t *running = (os_task_t *)osg.running;
    switch (running->status) {
    case OS_TASK_STATUS_ACTIVE:
        running->status = OS_TASK_STATUS_IDLE;
        break;
    case OS_TASK_STATUS_IDLE:
        // This would be weird?
        break;
    case OS_TASK_STATUS_WAIT:
        runqueue_remove(&osg.queue, running);
        waitqueue_add(&osg.waiting, running);
        #if defined(OS_CONFIG_DEBUG_SCHEDULE)
        os_printf("%s: waiting\n", running->name);
        #endif
        break;
    case OS_TASK_STATUS_SUSPENDED:
    case OS_TASK_STATUS_FINISHED:
        runqueue_remove(&osg.queue, running);
        break;
    }

    osg.scheduled = task;

    osi_stack_check();

    return OSS_SUCCESS;
}

#define OS_WAITQUEUE_NEXT_WRAPPED(n)   (((n)->nrp == NULL) ? osg.waiting : (n)->nrp)
#define OS_RUNQUEUE_NEXT_WRAPPED(n)    (((n)->nrp == NULL) ? osg.queue : (n)->nrp)

os_status_t osi_schedule() {
    os_task_t *new_task = NULL;

    // Check to see if anything in the waitqueue is free to go.
    uint32_t now = os_uptime();
    for (os_task_t *task = osg.waiting; task != NULL; task = task->nrp) {
        if (now >= task->delay) {
            new_task = task;
            break;
        }
    }

    // We always give just awoken tasks an immediate chance, for now.

    // Look for a task that's got the same priority or higher.
    if (new_task == NULL) {
        os_task_t *running = (os_task_t *)osg.running;
        os_priority_t ours = running->status == OS_TASK_STATUS_WAIT ? OS_PRIORITY_HIGHEST : running->priority;
        os_task_t *task = OS_RUNQUEUE_NEXT_WRAPPED(running);

        // Default to the running task just in case we don't go through the loop
        // or we're the only task, etc...
        new_task = task;

        // Look for a task that's higher priority than us (lower number)
        for ( ; task != running; task = OS_RUNQUEUE_NEXT_WRAPPED(task)) {
            if (ours >= task->priority) {
                new_task = task;
                break;
            }
            else {
                break;
            }
        }
    }

    osi_dispatch(new_task);

    OS_ASSERT(osg.running != NULL);
    OS_ASSERT(osg.scheduled != NULL);

    // Trigger PendSV!
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

    return OSS_SUCCESS;
}

inline uint32_t os_uptime() {
    return osi_platform_uptime();
}

extern char *sbrk(int32_t i);

uint32_t os_free_memory() {
    return (uint32_t)__get_MSP() - (uint32_t)sbrk(0);
}

uint32_t *osi_task_return_regs(os_task_t *task) {
    /* Get pointer to task return value registers (R0..R3) in Stack */
    #if defined(__SAMD51__)
    if (task->stack_kind == 1) {
        /* Extended Stack Frame: R4 - R11, S16 - S31, R0 - R3, R12, LR, PC,
         * xPSR, S0 - S15, FPSCR */
        return (uint32_t *)(task->sp + (8U * 4U) + (16U * 4U));
    }
    else {
        /* Basic Stack Frame: R4 - R11, R0 - R3, R12, LR, PC, xPSR */
        return (uint32_t *)(task->sp + (8U * 4U));
    }
    #else
    /* Stack Frame: R4 - R11, R0 - R3, R12, LR, PC, xPSR */
    return (uint32_t *)(task->sp + (8U * 4U));
    #endif
}

os_tuple_t *osi_task_return_tuple(os_task_t *task) {
    return (os_tuple_t *)osi_task_return_regs(task);
}

void osi_task_return_value(os_task_t *task, uint32_t v0) {
    uint32_t *regs = osi_task_return_regs(task);
    regs[0] = v0;
}

os_status_t osi_irs_systick() {
    if (osg.state == OS_STATE_STARTED) {
        return osi_schedule();
    }
    return OSS_SUCCESS;
}

inline void os_assert(const char *assertion, const char *file, int line) {
    os_printf("Assertion \"%s\" failed: file \"%s\", line %d\n", assertion, file, line);
    osi_error(OS_ERROR_ASSERTION);
}

void osi_error(uint8_t code) {
    __asm__("BKPT");
    infinite_loop();
}

void osi_stack_check() {
    if ((osg.running->sp < osg.running->stack) || (((uint32_t *)osg.running->stack)[0] != OSH_STACK_MAGIC_WORD)) {
        osi_error(OS_ERROR_STACK_OVERFLOW);
    }
}

void osi_hard_fault_report(uint32_t *stack, uint32_t lr, cortex_hard_fault_t *hfr) {
    volatile uint32_t looping = 1;
    while (looping) {
    }
}

void osi_hard_fault_handler(uint32_t *stack, uint32_t lr) {
    if (NVIC_HFSR & (1uL << 31)) {
        NVIC_HFSR |= (1uL << 31);         // Reset Hard Fault status
        *(stack + 6u) += 2u;              // PC is located on stack at SP + 24 bytes; increment PC by 2 to skip break instruction.
        return;                           // Return to application
    }

    cortex_hard_fault_t hfr;
    hfr.syshndctrl.byte = SYSHND_CTRL;    // System Handler Control and State Register
    hfr.mfsr.byte       = NVIC_MFSR;      // Memory Fault Status Register
    hfr.bfsr.byte       = NVIC_BFSR;      // Bus Fault Status Register
    hfr.bfar            = NVIC_BFAR;      // Bus Fault Manage Address Register
    hfr.ufsr.byte       = NVIC_UFSR;      // Usage Fault Status Register
    hfr.hfsr.byte       = NVIC_HFSR;      // Hard Fault Status Register
    hfr.dfsr.byte       = NVIC_DFSR;      // Debug Fault Status Register
    hfr.afsr            = NVIC_AFSR;      // Auxiliary Fault Status Register

    hfr.registers.R0 = (void *)stack[0];  // Register R0
    hfr.registers.R1 = (void *)stack[1];  // Register R1
    hfr.registers.R2 = (void *)stack[2];  // Register R2
    hfr.registers.R3 = (void *)stack[3];  // Register R3
    hfr.registers.R12 = (void *)stack[4]; // Register R12
    hfr.registers.LR = (void *)stack[5];  // Link register LR
    hfr.registers.PC = (void *)stack[6];  // Program counter PC
    hfr.registers.psr.byte = stack[7];    // Program status word PSR

    osi_hard_fault_report(stack, lr, &hfr);
}

static void task_finished() {
    OS_ASSERT(osg.running != NULL);
    OS_ASSERT(osg.running != osg.idle);
    OS_ASSERT(osg.running->status == OS_TASK_STATUS_ACTIVE);

    os_printf("os: task '%s' finished\n", osg.running->name);

    osg.running->status = OS_TASK_STATUS_FINISHED;

    infinite_loop();
}

static void infinite_loop() {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

static uint32_t runqueue_length(os_task_t *head) {
    uint32_t l = 0;

    for ( ; head != NULL; head = head->nrp) {
        l++;
    }

    return l;
}

static void runqueue_add(os_task_t **head, os_task_t *task) {
    task->nrp = NULL;

    if (*head == NULL) {
        *head = task;
        return;
    }

    os_task_t *previous = NULL;
    for (os_task_t *iter = *head; iter != NULL; iter = iter->nrp) {
        if (task->priority < iter->priority) {
            task->nrp = iter;
            if (previous == NULL) {
                *head = task;
            }
            else {
                previous->nrp = task;
            }
            return;
        }
        if (iter->nrp == NULL) {
            iter->nrp = task;
            return;
        }
        previous  = iter;
    }

    OS_ASSERT(0);
}
static void runqueue_remove(os_task_t **head, os_task_t *task) {
    os_task_t *previous = NULL;

    for (os_task_t *iter = *head; iter != NULL; iter = iter->nrp) {
        if (iter == task) {
            if (*head == iter) {
                *head = iter->nrp;
            }
            else {
                previous->nrp = iter->nrp;
            }
            iter->nrp = NULL;
            break;
        }
        previous = iter;
    }
}

static void waitqueue_add(os_task_t **head, os_task_t *task) {
    task->nrp = NULL;

    if (*head == NULL) {
        *head = task;
        return;
    }

    os_task_t *previous = NULL;
    for (os_task_t *iter = *head; iter != NULL; iter = iter->nrp) {
        if (task->delay < iter->delay) {
            task->nrp = iter;
            if (previous == NULL) {
                *head = task;
            }
            else {
                previous->nrp = task;
            }
            return;
        }
        if (iter->nrp == NULL) {
            iter->nrp = task;
            return;
        }
        previous  = iter;
    }

    OS_ASSERT(0);
}

static void waitqueue_remove(os_task_t **head, os_task_t *task) {
    os_task_t *previous = NULL;

    for (os_task_t *iter = *head; iter != NULL; iter = iter->nrp) {
        if (iter == task) {
            if (*head == iter) {
                *head = iter->nrp;
            }
            else {
                previous->nrp = iter->nrp;
            }
            iter->nrp = NULL;
            break;
        }
        previous = iter;
    }
}
