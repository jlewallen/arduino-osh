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

#include <cinttypes>
#include <cassert>
#include <Arduino.h>

#include "os.h"

typedef enum {
    OS_STATE_DEFAULT = 1,
    OS_STATE_INITIALIZED,
    OS_STATE_TASKS_INITIALIZED,
    OS_STATE_STARTED,
} os_state;

typedef struct os_system_t {
    os_state state;
    os_task_t *tasks;
    bool initialized;
} os_system_t;

os_system_t oss{
    OS_STATE_DEFAULT,
    nullptr,
    false
};

volatile os_task_t *running_task;
volatile os_task_t *scheduled_task;

static void task_finished() {
    /* This function is called when some task handler returns. */
    volatile uint32_t i = 0;

    while (1) {
        i++;
    }
}

bool os_initialize() {
    if (oss.state != OS_STATE_DEFAULT) {
        return false;
    }

    oss.state = OS_STATE_INITIALIZED;

    return true;
}

bool os_task_initialize(os_task_t *task, void (*handler)(void *params), void *params, uint32_t *stack, size_t stack_size) {
    if (oss.state != OS_STATE_INITIALIZED && oss.state != OS_STATE_TASKS_INITIALIZED) {
        return false;
    }

    if ((stack_size % sizeof(uint32_t)) != 0) /* TODO: Use assert? */ {
        return false;
    }

    uint32_t stack_offset = (stack_size / sizeof(uint32_t));

    /* Initialize the task structure and set SP to the top of the stack
       minus 16 words (64 bytes) to leave space for storing 16 registers: */
    task->handler = handler;
    task->np = nullptr;
    task->params = params;
    task->sp = (uint32_t)(stack + stack_offset - 16);
    task->status = OS_TASK_STATUS_IDLE;

    /* Save init. values of registers which will be restored on exc. return:
       - XPSR: Default value (0x01000000)
       - PC: Point to the handler function
       - LR: Point to a function to be called when the handler returns
       - R0: Point to the handler function's parameter */
    stack[stack_offset - 1] = 0x01000000;
    stack[stack_offset - 2] = (uint32_t)handler;
    stack[stack_offset - 3] = (uint32_t)&task_finished;
    stack[stack_offset - 8] = (uint32_t)params;

    #ifdef OS_CONFIG_DEBUG
    uint32_t base = 1000;
    stack[stack_offset - 4] = base + 12;  /* R12 */
    stack[stack_offset - 5] = base + 3;   /* R3  */
    stack[stack_offset - 6] = base + 2;   /* R2  */
    stack[stack_offset - 7] = base + 1;   /* R1  */
    /* p_stack[stack_offset-8] is R0 */
    stack[stack_offset - 9] = base + 7;   /* R7  */
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

    if (running_task == nullptr) {
        running_task = oss.tasks;
        scheduled_task = nullptr;
    }

    oss.state = OS_STATE_TASKS_INITIALIZED;

    return true;
}

bool os_start(void) {
    if (oss.state != OS_STATE_TASKS_INITIALIZED) {
        return false;
    }

    NVIC_SetPriority(PendSV_IRQn, 0xff);
    NVIC_SetPriority(SysTick_IRQn, 0x00);

    oss.state = OS_STATE_STARTED;

    __set_PSP(running_task->sp + 64); /* Set PSP to the top of task's stack */
    __set_CONTROL(0x03); /* Switch to Unprivilleged Thread Mode with PSP */
    __ISB(); /* Execute ISB after changing CONTORL (recommended) */

    oss.initialized = true;

    running_task->handler(running_task->params);

    return true;
}

extern "C" {

extern void SysTick_DefaultHandler(void);

int sysTickHook() {
    SysTick_DefaultHandler();

    if (!oss.initialized) {
        return 1;
    }

    // Select next task.. pretty straightforward.
    if (running_task->np != nullptr) {
        scheduled_task = running_task->np;
    }
    else {
        scheduled_task = oss.tasks;
    }

    // Should this happen in the PendSV?
    running_task->status = OS_TASK_STATUS_IDLE;
    scheduled_task->status = OS_TASK_STATUS_ACTIVE;

    // Trigger PendSV!
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

    return 1;
}

void HardFault_Handler() {
    Serial.println("HF");

    while (true) {
    }
}

void PendSV_Handler() {
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
        "mov  r3, #0\n"
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

        /* EXC_RETURN - Thread mode with PSP: */
        "ldr r0, =0xFFFFFFFD\n"

        /* Enable interrupts: */
        "cpsie	i\n"

        "bx	r0\n"
        );
}

}
