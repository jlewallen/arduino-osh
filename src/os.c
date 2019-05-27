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

typedef struct os_stack_frame_t {
    uint32_t xpsr;
    void *pc;
    void *lr;
    uint32_t r12;
    uint32_t r3;
    uint32_t r2;
    uint32_t r1;
    uint32_t r0;
    uint32_t r7;
    uint32_t r6;
    uint32_t r5;
    uint32_t r4;
    uint32_t r11;
    uint32_t r10;
    uint32_t r9;
    uint32_t r8;
} os_stack_frame_t;

uint32_t *initialize_stack(os_task_t *task, uint32_t *stack, size_t stack_size) {
    OSDOTH_ASSERT((stack_size % sizeof(uint32_t)) == 0);
    OSDOTH_ASSERT(stack_size >= OSDOTH_STACK_MINIMUM_SIZE);

    uint32_t stack_offset = (stack_size / sizeof(uint32_t));
    for (uint32_t i = 0; i < stack_offset; ++i) {
        stack[i] = OSH_STACK_MAGIC_PATTERN;
    }

    // Check alignment. May not be necessary?
    uint32_t *stk = stack + stack_offset;
    if ((uint32_t)stk & 0x04U) {
        stk--;
    }
    stk -= OSDOTH_STACK_BASIC_FRAME_SIZE;

    /* Save values of registers which will be restored on exc. return:
       - XPSR: Default value (0x01000000)
       - PC: Point to the handler function
       - LR: Point to a function to be called when the handler returns
       - R0: Point to the handler function's parameter */
    stk[15] = 0x01000000;
    stk[14] = (uint32_t)task->handler & ~0x01UL;
    stk[13] = (uint32_t)&task_finished;
    stk[ 8] = (uint32_t)task->params;
    #if defined(OSDOTH_CONFIG_DEBUG)
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
    task->blocked = NULL;
    #if defined(OSDOTH_CONFIG_DEBUG)
    task->debug_stack_max = 0;
    #endif

    task->sp = initialize_stack(task, stack, stack_size);

    task->np = osg.tasks;
    osg.tasks = task;
    osg.ntasks++;

    /* First task initialized is always the idle task. */
    if (osg.idle == NULL) {
        OSDOTH_ASSERT(task->status != OS_TASK_STATUS_SUSPENDED);
        osg.idle = task;
    }

    /* This will always initialize the initial task to be the idle task, this
     * that's the first call to this. */
    if (osg.running == NULL) {
        osg.running = osg.tasks;
        osg.scheduled = NULL;
        osi_stack_check();
    }

    osg.state = OS_STATE_TASKS_INITIALIZED;

    return OSS_SUCCESS;
}

os_status_t os_task_start(os_task_t *task) {
    OSDOTH_ASSERT(task != NULL);
    OSDOTH_ASSERT(task->status != OS_TASK_STATUS_IDLE && task->status != OS_TASK_STATUS_ACTIVE);

    task->stack_kind = 0;
    task->delay = 0;
    task->flags = 0;
    task->queue = NULL;
    task->mutex = NULL;
    task->blocked = NULL;
    task->started = os_uptime();
    #if defined(OSDOTH_CONFIG_DEBUG)
    task->debug_stack_max = 0;
    #endif
    task->sp = initialize_stack(task, (uint32_t *)task->stack, task->stack_size);
    task->status = OS_TASK_STATUS_IDLE;

    return OSS_SUCCESS;
}

os_status_t os_task_suspend(os_task_t *task) {
    OSDOTH_ASSERT(task != NULL);
    OSDOTH_ASSERT(task->status == OS_TASK_STATUS_IDLE || task->status == OS_TASK_STATUS_ACTIVE);

    task->status = OS_TASK_STATUS_SUSPENDED;

    return OSS_SUCCESS;
}

os_status_t os_task_resume(os_task_t *task) {
    OSDOTH_ASSERT(task != NULL);
    OSDOTH_ASSERT(task->status == OS_TASK_STATUS_SUSPENDED);

    task->status = OS_TASK_STATUS_IDLE;

    return OSS_SUCCESS;
}

uint32_t os_task_uptime(os_task_t *task) {
    OSDOTH_ASSERT(task != NULL);
    return os_uptime() - task->started;
}

uint32_t os_task_runtime(os_task_t *task) {
    OSDOTH_ASSERT(task != NULL);
    return os_uptime() - task->started;
}

os_task_status os_task_get_status(os_task_t *task) {
    return task->status;
}

os_status_t os_start(void) {
    if (osg.state != OS_STATE_TASKS_INITIALIZED) {
        return OSS_ERROR_INVALID;
    }

    OSDOTH_ASSERT(osi_platform_setup() == OSS_SUCCESS);
    OSDOTH_ASSERT(osg.running != NULL);

    NVIC_SetPriority(PendSV_IRQn, 0xff);
    NVIC_SetPriority(SysTick_IRQn, 0x00);

    // uint32_t stack[8];
    // __set_PSP((uint32_t)(stack + 8));

    /* Set PSP to the top of task's stack */
    __set_PSP((uint32_t)osg.running->sp + OSDOTH_STACK_BASIC_FRAME_SIZE);
    /* Switch to Unprivilleged Thread Mode with PSP */
    __set_CONTROL(0x03);
    /* Execute DSB/ISB after changing CONTORL (recommended) */
    __DSB();
    __ISB();

    osg.state = OS_STATE_STARTED;
    osg.running->handler(osg.running->params);

    OSDOTH_ASSERT(0);

    return true;
}

os_status_t osi_dispatch(os_task_t *task) {
    OSDOTH_ASSERT(task != NULL);
    OSDOTH_ASSERT(osg.running != NULL);
    OSDOTH_ASSERT(task != osg.running);

    task->flags = 0;

    osg.scheduled = task;

    // NOTE: Should the status update happen in the PendSV?
    switch (osg.running->status) {
    case OS_TASK_STATUS_ACTIVE:
        osg.running->status = OS_TASK_STATUS_IDLE;
        break;
    case OS_TASK_STATUS_IDLE:
    case OS_TASK_STATUS_WAIT:
    case OS_TASK_STATUS_SUSPENDED:
    case OS_TASK_STATUS_FINISHED:
        break;
    }
    osg.scheduled->status = OS_TASK_STATUS_ACTIVE;

    osi_stack_check();

    return OSS_SUCCESS;
}

os_status_t osi_schedule() {
    os_task_t *running = os_task_self();

    /* May be unnecessary for us to be here... */
    osg.scheduled = NULL;

    #if defined(OSDOTH_CONFIG_DEBUG)
    /* Calculate stack usage and update when debugging. */
    uint32_t stack_usage = os_task_stack_usage((os_task_t *)running);
    if (stack_usage > running->debug_stack_max) {
        running->debug_stack_max = stack_usage;
    }
    #endif

    // Schedule a new task to run...
    os_task_t *iter = running;
    while (true) {
        if (iter->np != NULL) {
            iter = iter->np;
        }
        else {
            iter = osg.tasks;
        }

        // If no other tasks can run but the one that just did, just return.
        // Technically, this should only happen to the idle task.
        if (iter == osg.running) {
            OSDOTH_ASSERT(iter == osg.idle);
            return OSS_SUCCESS;
        }

        // Wake up tasks that are waiting if it's their time.
        if (iter->status == OS_TASK_STATUS_WAIT) {
            if (os_uptime() >= iter->delay) {
                iter->status = OS_TASK_STATUS_IDLE;
                if ((iter->flags & OS_TASK_FLAG_MUTEX) == OS_TASK_FLAG_MUTEX) {
                    OSDOTH_ASSERT(iter->queue == NULL);
                    OSDOTH_ASSERT(iter->mutex != NULL);
                    OSDOTH_ASSERT(iter->mutex->blocked == iter);

                    #if defined(OSDOTH_CONFIG_DEBUG_MUTEXES)
                    os_printf("%s: removed from mutex %p\n", iter->name, iter->mutex);
                    #endif

                    iter->mutex->blocked = iter->mutex->blocked->blocked;
                    iter->blocked = NULL;
                    iter->queue = NULL;
                    iter->mutex = NULL;
                    iter->flags = 0;
                }
                if ((iter->flags & OS_TASK_FLAG_QUEUE) == OS_TASK_FLAG_QUEUE) {
                    OSDOTH_ASSERT(iter->mutex == NULL);
                    OSDOTH_ASSERT(iter->queue != NULL);
                    OSDOTH_ASSERT(iter->queue->blocked == iter);

                    #if defined(OSDOTH_CONFIG_DEBUG_QUEUES)
                    os_printf("%s: removed from queue %p\n", iter->name, iter->queue);
                    #endif

                    iter->queue->blocked = iter->queue->blocked->blocked;
                    iter->blocked = NULL;
                    iter->queue = NULL;
                    iter->mutex = NULL;
                    iter->flags = 0;
                }
                iter->delay = 0;
                osi_dispatch(iter);
                break;
            }
        }

        // Only run tasks that are idle.
        if (iter->status == OS_TASK_STATUS_IDLE) {
            osi_dispatch(iter);
            break;
        }
    }

    OSDOTH_ASSERT(osg.running != NULL);
    OSDOTH_ASSERT(osg.scheduled != NULL);

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
    OSDOTH_ASSERT(osg.running != NULL);
    OSDOTH_ASSERT(osg.running != osg.idle);
    OSDOTH_ASSERT(osg.running->status == OS_TASK_STATUS_ACTIVE);

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
