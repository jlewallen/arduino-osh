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

#include <inttypes.h>
#include <sam.h>

#include "os.h"
#include "faults.h"

#if defined(OSH_MTB)
__attribute__((__aligned__(DEBUG_MTB_SIZE_BYTES))) uint32_t mtb[DEBUG_MTB_SIZE];
#endif

#define OSH_STACK_MAGIC_WORD     0xE25A2EA5U
#define OSH_STACK_MAGIC_PATTERN  0xCCCCCCCCU

typedef enum {
    OS_STATE_DEFAULT = 1,
    OS_STATE_INITIALIZED,
    OS_STATE_TASKS_INITIALIZED,
    OS_STATE_STARTED,
} os_state;

typedef struct os_system_t {
    os_state state;
    uint8_t ntasks;
    os_task_t *tasks;
} os_system_t;

os_system_t oss = {
    OS_STATE_DEFAULT,
    0,
    NULL
};

// TODO: Move these to os_system_t, need to work on my assembly first, though.
volatile os_task_t *running_task = NULL;
volatile os_task_t *scheduled_task = NULL;

static void infinite_loop()  __attribute__ ((noreturn));

static void task_finished()  __attribute__ ((noreturn));

bool os_initialize() {
    if (oss.state != OS_STATE_DEFAULT) {
        return false;
    }

    #if defined(OSH_MTB)
    REG_MTB_POSITION = ((uint32_t)(mtb - REG_MTB_BASE)) & 0xFFFFFFF8;
    REG_MTB_FLOW = (((uint32_t)mtb - REG_MTB_BASE) + DEBUG_MTB_SIZE_BYTES) & 0xFFFFFFF8;
    REG_MTB_MASTER = 0x80000000 + (DEBUG_MTB_MAGNITUDE_PACKETS - 1);
    #endif

    oss.state = OS_STATE_INITIALIZED;

    return true;
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

bool os_task_initialize(os_task_t *task, void (*handler)(void *params), void *params, uint32_t *stack, size_t stack_size) {
    if (oss.state != OS_STATE_INITIALIZED && oss.state != OS_STATE_TASKS_INITIALIZED) {
        return false;
    }

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
    stk[14] = (uint32_t)handler & ~0x01UL;
    stk[13] = (uint32_t)&task_finished;
    stk[ 8] = (uint32_t)params;
    #if defined(OSDOTH_CONFIG_DEBUG)
    uint32_t base = 1000 * (oss.ntasks + 1);
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

    task->sp = stk;
    task->stack = stack;
    task->stack_size = stack_size;
    task->params = params;
    task->handler = handler;
    task->status = OS_TASK_STATUS_IDLE;
    task->np = oss.tasks;

    oss.tasks = task;
    oss.ntasks++;

    /* This will always initialize the initial task to be the idle task, this
     * that's the first call to this. */
    if (running_task == NULL) {
        running_task = oss.tasks;
        scheduled_task = NULL;
        os_stack_check();
    }

    oss.state = OS_STATE_TASKS_INITIALIZED;

    return true;
}

bool os_task_suspend(os_task_t *task) {
    OSDOTH_ASSERT(task->status == OS_TASK_STATUS_IDLE || task->status == OS_TASK_STATUS_ACTIVE);

    task->status = OS_TASK_STATUS_SUSPENDED;

    return true;
}

bool os_task_resume(os_task_t *task) {
    OSDOTH_ASSERT(task->status == OS_TASK_STATUS_SUSPENDED);

    task->status = OS_TASK_STATUS_IDLE;

    return true;
}

os_task_status os_task_get_status(os_task_t *task) {
    return task->status;
}

bool os_start(void) {
    if (oss.state != OS_STATE_TASKS_INITIALIZED) {
        return false;
    }

    OSDOTH_ASSERT(os_platform_setup());

    OSDOTH_ASSERT(running_task != NULL);

    NVIC_SetPriority(PendSV_IRQn, 0xff);
    NVIC_SetPriority(SysTick_IRQn, 0x00);

    // uint32_t stack[8];
    // __set_PSP((uint32_t)(stack + 8));

    /* Set PSP to the top of task's stack */
    __set_PSP((uint32_t)running_task->sp + OSDOTH_STACK_BASIC_FRAME_SIZE);
    /* Switch to Unprivilleged Thread Mode with PSP */
    __set_CONTROL(0x03);
    /* Execute DSB/ISB after changing CONTORL (recommended) */
    __DSB();
    __ISB();

    oss.state = OS_STATE_STARTED;

    running_task->handler(running_task->params);

    OSDOTH_ASSERT(0);

    infinite_loop();

    return true;
}

static void task_finished() {
    OSDOTH_ASSERT(running_task != NULL);
    OSDOTH_ASSERT(running_task->status == OS_TASK_STATUS_ACTIVE);

    os_printf("os: task finished\n");

    running_task->status = OS_TASK_STATUS_FINISHED;
    oss.ntasks--;

    infinite_loop();
}

static void os_schedule() {
    /* May be unnecessary for us to be here... */
    scheduled_task = NULL;

    // Schedule a new task to run...
    volatile os_task_t *iter = running_task;
    while (true) {
        if (iter->np != NULL) {
            iter = iter->np;
        }
        else {
            iter = oss.tasks;
        }

        // If no other tasks can run but the one that just did, go ahead.
        if (iter == running_task) {
            return;
        }

        // Only run tasks that are idle.
        if (iter->status == OS_TASK_STATUS_IDLE) {
            scheduled_task = iter;
            break;
        }
    }

    OSDOTH_ASSERT(running_task != NULL);
    OSDOTH_ASSERT(scheduled_task != NULL);

    // NOTE: Should this happen in the PendSV?
    if (running_task->status != OS_TASK_STATUS_FINISHED) {
        running_task->status = OS_TASK_STATUS_IDLE;
    }
    scheduled_task->status = OS_TASK_STATUS_ACTIVE;

    os_stack_check();

    // Trigger PendSV!
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void os_log(const char *f, ...) {
}

void os_error(uint8_t code) {
    infinite_loop();
}

void os_stack_check() {
    if ((running_task->sp < running_task->stack) || (((uint32_t *)running_task->stack)[0] != OSH_STACK_MAGIC_WORD)) {
        os_error(0);
    }
}

void os_delay(uint32_t ms) {
    os_platform_delay(ms);
}

void os_irs_systick() {
    if (oss.state == OS_STATE_STARTED) {
        os_schedule();
    }
}

void hard_fault_handler(uint32_t *stack, uint32_t lr) {
    if (NVIC_HFSR & (1uL << 31)) {
        NVIC_HFSR |= (1uL << 31);   // Reset Hard Fault status
        *(stack + 6u) += 2u;        // PC is located on stack at SP + 24 bytes; increment PC by 2 to skip break instruction.
        return;                     // Return to application
    }

    cortex_hard_fault_t hfr;
    hfr.syshndctrl.byte = SYSHND_CTRL;  // System Handler Control and State Register
    hfr.mfsr.byte       = NVIC_MFSR;    // Memory Fault Status Register
    hfr.bfsr.byte       = NVIC_BFSR;    // Bus Fault Status Register
    hfr.bfar            = NVIC_BFAR;    // Bus Fault Manage Address Register
    hfr.ufsr.byte       = NVIC_UFSR;    // Usage Fault Status Register
    hfr.hfsr.byte       = NVIC_HFSR;    // Hard Fault Status Register
    hfr.dfsr.byte       = NVIC_DFSR;    // Debug Fault Status Register
    hfr.afsr            = NVIC_AFSR;    // Auxiliary Fault Status Register

    hfr.registers.R0 = stack[0];        // Register R0
    hfr.registers.R1 = stack[1];        // Register R1
    hfr.registers.R2 = stack[2];        // Register R2
    hfr.registers.R3 = stack[3];        // Register R3
    hfr.registers.R12 = stack[4];       // Register R12
    hfr.registers.LR = stack[5];        // Link register LR
    hfr.registers.PC = stack[6];        // Program counter PC
    hfr.registers.psr.byte = stack[7];  // Program status word PSR

    volatile uint32_t looping = 1;
    while (looping) {
    }
}

inline static void infinite_loop() {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}
