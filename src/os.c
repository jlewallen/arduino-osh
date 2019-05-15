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

/* Allocate minimum stack . */
static uint32_t idle_stack[OSDOTH_STACK_MINIMUM_SIZE_WORDS + OSDOTH_STACK_FUNCTION_OVERHEAD_WORDS];

static os_task_t idle_task;

static void infinite_loop()  __attribute__ ((noreturn));

static void task_idle()  __attribute__ ((noreturn));

static void task_finished()  __attribute__ ((noreturn));

bool os_initialize() {
    if (oss.state != OS_STATE_DEFAULT) {
        return false;
    }

    oss.state = OS_STATE_INITIALIZED;

    OSDOTH_ASSERT(os_task_initialize(&idle_task, &task_idle, NULL, idle_stack, sizeof(idle_stack)));

    return true;
}

bool os_task_initialize(os_task_t *task, void (*handler)(void *params), void *params, uint32_t *stack, size_t stack_size) {
    if (oss.state != OS_STATE_INITIALIZED && oss.state != OS_STATE_TASKS_INITIALIZED) {
        return false;
    }

    OSDOTH_ASSERT((stack_size % sizeof(uint32_t)) == 0);

    OSDOTH_ASSERT(stack_size >= OSDOTH_STACK_MINIMUM_SIZE);

    uint32_t stack_offset = (stack_size / sizeof(uint32_t));

    /* Initialize the task structure and set SP to the top of the stack
       minus OSDOTH_STACK_MINIMUM_SIZE_WORDS words (OSDOTH_STACK_MINIMUM_SIZE
       bytes) to leave space for storing 16 registers: */
    task->sp = (uint32_t)(stack + stack_offset - OSDOTH_STACK_MINIMUM_SIZE_WORDS);
    task->params = params;
    task->handler = handler;
    task->np = NULL;
    task->status = OS_TASK_STATUS_IDLE;

    /* Save values of registers which will be restored on exc. return:
       - XPSR: Default value (0x01000000)
       - PC: Point to the handler function
       - LR: Point to a function to be called when the handler returns
       - R0: Point to the handler function's parameter */
    stack[stack_offset - 1] = 0x01000000;
    stack[stack_offset - 2] = (uint32_t)handler & ~0x01UL;
    stack[stack_offset - 3] = (uint32_t)&task_finished;
    stack[stack_offset - 8] = (uint32_t)params;

    #if defined(OSDOTH_CONFIG_DEBUG)
    uint32_t base = 1000 * (oss.ntasks + 1);
    stack[stack_offset -  4] = base + 12; /* R12 */
    stack[stack_offset -  5] = base + 3;  /* R3  */
    stack[stack_offset -  6] = base + 2;  /* R2  */
    stack[stack_offset -  7] = base + 1;  /* R1  */
    /* stack[stack_offset - 8] is R0 */
    stack[stack_offset -  9] = base + 7;  /* R7  */
    stack[stack_offset - 10] = base + 6;  /* R6  */
    stack[stack_offset - 11] = base + 5;  /* R5  */
    stack[stack_offset - 12] = base + 4;  /* R4  */
    stack[stack_offset - 13] = base + 11; /* R11 */
    stack[stack_offset - 14] = base + 10; /* R10 */
    stack[stack_offset - 15] = base + 9;  /* R9  */
    stack[stack_offset - 16] = base + 8;  /* R8  */
    #endif /* OS_CONFIG_DEBUG */

    task->np = oss.tasks;
    oss.tasks = task;
    oss.ntasks++;

    /* This will always initialize the initial task to be the idle task, this
     * that's the first call to this. */
    if (running_task == NULL) {
        running_task = oss.tasks;
        scheduled_task = NULL;
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
    __set_PSP(running_task->sp + OSDOTH_STACK_MINIMUM_SIZE);
    /* Switch to Unprivilleged Thread Mode with PSP */
    __set_CONTROL(0x02);
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

    os_log("os: task finished");

    running_task->status = OS_TASK_STATUS_FINISHED;
    oss.ntasks--;

    infinite_loop();
}

static void task_idle() {
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

    // Trigger PendSV!
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void os_log(const char *f, ...) {
}

void os_irs_systick() {
    if (oss.state == OS_STATE_STARTED) {
        os_schedule();
    }
}

OS_DECLARE_HARD_FAULT_HANDLER() {
    infinite_loop();
}

OS_DECLARE_PENDSV_HANDLER() {
    asm(
        ".syntax unified\n"
        #if defined(__SAMD21__)
        ".cpu cortex-m0\n"
        ".fpu softvfp\n"
        #endif
        #if defined(__SAMD51__)
        ".cpu cortex-m4\n"
        #endif

        ".thumb\n"

        /* Disable interrupts: */
        "cpsid	i\n"

        /* Check to see if we're switching to the same task. */
        "ldr     r1, =running_task\n"
        "ldr     r1, [r1]\n"
        "ldr     r2, =scheduled_task\n"
        "ldr     r2, [r2]\n"
        "cmp     r1, r2\n"
        "beq     pendsv_done\n"

        /*
          Exception frame saved by the NVIC hardware onto stack:
          +------+
          |      | <- SP before interrupt (orig. SP)
          | xPSR |
          |  PC  |
          |  LR  |
          |  R12 |
          |  R3  |
          |  R2  |
          |  R1  |
          |  R0  | <- SP after entering interrupt (orig. SP + 32 bytes)
          +------+

          Registers saved by the software (PendSV_Handler):
          +------+
          |  R7  |
          |  R6  |
          |  R5  |
          |  R4  |
          |  R11 |
          |  R10 |
          |  R9  |
          |  R8  | <- Saved SP (orig. SP + 64 bytes)
          +------+
        */

        /* Save registers R4-R11 (32 bytes) onto current PSP (process stack
           pointer) and make the PSP point to the last stacked register (R8):
           - The MRS/MSR instruction is for loading/saving a special registers.
           - The STMIA inscruction can only save low registers (R0-R7), it is
           therefore necesary to copy registers R8-R11 into R4-R7 and call
           STMIA twice. */
        "mrs	  r0, psp\n"
        "subs	  r0, #16\n"
        "stmia	r0!, {r4-r7}\n"
        "mov	  r4, r8\n"
        "mov	  r5, r9\n"
        "mov	  r6, r10\n"
        "mov	  r7, r11\n"
        "subs	  r0, #32\n"
        "stmia	r0!, {r4-r7}\n"
        "subs	  r0, #16\n"

        /* Save current task's SP: */
        "ldr	r2, =running_task\n"
        "ldr	r1, [r2]\n"
        "str	r0, [r1]\n"

        /* running_task = scheduled_task; */
        "ldr	r2, =scheduled_task\n"
        "ldr  r2, [r2]\n"
        "ldr	r3, =running_task\n"
        "str  r2, [r3]\n"

        /* Load next task's SP: */
        "ldr	r2, =scheduled_task\n"
        "ldr	r1, [r2]\n"
        "ldr	r0, [r1]\n"
        /* scheduled_task = 0; */
        "movs r3, #0\n"
        "str  r3, [r2]\n"

        /* Load registers R4-R11 (32 bytes) from the new PSP and make the PSP
           point to the end of the exception stack frame. The NVIC hardware
           will restore remaining registers after returning from exception): */
        "ldmia	r0!, {r4-r7}\n"
        "mov	  r8, r4\n"
        "mov	  r9, r5\n"
        "mov	  r10, r6\n"
        "mov  	r11, r7\n"
        "ldmia	r0!, {r4-r7}\n"
        "msr	  psp, r0\n"

        "pendsv_done:\n"
        /* EXC_RETURN - Thread mode with PSP: */
        "ldr r0, =0xFFFFFFFD\n"

        /* Enable interrupts: */
        "cpsie	i\n"

        "bx	r0\n"
    );
}

inline static void infinite_loop() {
    while (true) {
        // #if defined(REG_MTB_MASTER)
        // REG_MTB_MASTER = 0x00000000;
        // #endif
        asm(
            #if defined(__SAMD21__)
            ".cpu cortex-m0\n"
            ".fpu softvfp\n"
            #endif
            #if defined(__SAMD51__)
            ".cpu cortex-m4\n"
            #endif

            ".thumb\n"
            "1:\n"
            "b 1b\n"
        );
    }
}
