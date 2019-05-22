.file   "gcc.s"
.syntax unified
.thumb

.equ    OS_TASK_STACK_KIND, 12
.equ    OS_TASK_SP, 0

.section ".text"
.align  2

#if defined(__SAMD21__)
/*-------------------------- HardFault_Handler ---------------------------------*/

        .thumb_func
        .type   HardFault_Handler, %function
        .global HardFault_Handler
HardFault_Handler:
        .fnstart
        .cantunwind

        movs   r2, #4
        mov    r1, lr
        tst    r0, r2
        bne    hf_uses_psp
        mrs    r0, msp
        b      hf_pass_stack_ptr
        hf_uses_psp:
        mrs    r0, psp
        hf_pass_stack_ptr:
        mov    r1, lr
        b      hard_fault_handler

        .fnend
        .size   HardFault_Handler, .-HardFault_Handler

/*-------------------------- PendSV_Handler ---------------------------------*/

        .thumb_func
        .type   PendSV_Handler, %function
        .global PendSV_Handler
PendSV_Handler:
        .fnstart
        .cantunwind

        /* Disable interrupts: */
        cpsid	i

        /* Check to see if we're switching to the same task. */
        ldr     r1, =running_task
        ldr     r1, [r1]
        ldr     r2, =scheduled_task
        ldr     r2, [r2]
        cmp     r1, r2
        beq     pendsv_done

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
        mrs	  r0, psp
        subs	  r0, #16
        stmia	r0!, {r4-r7}
        mov	  r4, r8
        mov	  r5, r9
        mov	  r6, r10
        mov	  r7, r11
        subs	  r0, #32
        stmia	r0!, {r4-r7}
        subs	  r0, #16

        /* Save current task's SP: */
        ldr	r2, =running_task
        ldr	r1, [r2]
        str	r0, [r1]

        /* check for stack overflow */
        push {r2, r3}
        bl   os_stack_check
        pop  {r2, r3}

        /* running_task = scheduled_task; */
        ldr	r2, =scheduled_task
        ldr  r2, [r2]
        ldr	r3, =running_task
        str  r2, [r3]

        /* check for stack overflow */
        push {r2, r3}
        bl   os_stack_check
        pop  {r2, r3}

        /* Load next task's SP: */
        ldr	r2, =scheduled_task
        ldr	r1, [r2]
        ldr	r0, [r1]
        /* scheduled_task = 0; */
        movs r3, #0
        str  r3, [r2]

        /* Load registers R4-R11 (32 bytes) from the new PSP and make the PSP
           point to the end of the exception stack frame. The NVIC hardware
           will restore remaining registers after returning from exception): */
        ldmia	r0!, {r4-r7}
        mov	  r8, r4
        mov	  r9, r5
        mov	  r10, r6
        mov  	r11, r7
        ldmia	r0!, {r4-r7}
        msr	  psp, r0

        pendsv_done:
        /* EXC_RETURN - Thread mode with PSP: */
        ldr r0, =0xFFFFFFFD

        /* Enable interrupts: */
        cpsie	i

        bx	r0

#endif

#if defined(__SAMD51__)
/*-------------------------- HardFault_Handler ---------------------------------*/

        .thumb_func
        .type   HardFault_Handler, %function
        .global HardFault_Handler
HardFault_Handler:
        .fnstart
        .cantunwind

        tst    lr, #4
        ite    EQ
        mrseq  r0, msp
        mrsne  r0, psp
        mov    r1, lr
        b      hard_fault_handler

        .fnend
        .size   HardFault_Handler, .-HardFault_Handler

/*-------------------------- PendSV_Handler ---------------------------------*/

        .thumb_func
        .type   PendSV_Handler, %function
        .global PendSV_Handler
PendSV_Handler:
        .fnstart
        .cantunwind

        /* Check to see if we're switching to the same task. */
        ldr      r1, =running_task
        ldr      r1, [r1]
        ldr      r2, =scheduled_task
        ldr      r2, [r2]
        cmp      r1, r2
        beq      pendsv_done

        /* Disable interrupts: */
        cpsid	i

        mrs      r12, psp                   /* Read PSP */
        tst      lr, #0x10                  /* is it extended frame? */
        itte     eq
        vstmdbeq r12!, {s16 - s31}          /* yes, stack also VFP hi-regs */
        moveq    r0, #0x01                  /* os_tsk->stack_frame val */
        movne    r0, #0x00
        strb     r0, [r1, #OS_TASK_STACK_KIND]      /* os_tsk.run->stack_frame = val */
        stmdb    r12!, {r4 - r11}           /* Save Old context */
        str      r12, [r1, #OS_TASK_SP]     /* Update os_tsk.run->tsk_stack */

        push     {r2, r3}
        bl       os_stack_check
        pop      {r2, r3}

        /* running_task = scheduled_task; */
        ldr      r2, =scheduled_task
        ldr      r2, [r2]
        ldr      r3, =running_task
        str      r2, [r3]
        /* scheduled_task = 0; */
        ldr      r2, =scheduled_task
        movs     r3, #0
        str      r3, [r2]
        movs     r2, #0

        ldr      r3, =running_task
        ldr      r3, [r3]

        ldr      r12, [r3, #OS_TASK_SP]     /* os_tsk.next->tsk_stack */
        ldmia    r12!, {r4 - r11}           /* Restore New Context */
        ldrb     r0, [r3, #OS_TASK_STACK_KIND]      /* Stack Frame */
        cmp      r0, #0                     /* Basic/Extended Stack Frame */
        itee     eq
        mvneq    lr, #~0xFFFFFFFD           /* set EXC_RETURN value */
        mvnne    lr, #~0xFFFFFFED
        vldmiane r12!, {s16 - s31}          /* restore VFP hi-registers */
        msr      psp, r12                   /* Write PSP */

        /* Enable interrupts: */
        cpsie	i

pendsv_done:
        .ifdef   IFX_XMC4XXX
        push     {lr}
        pop      {pc}
        .else
        bx       lr                         /* Return to Thread Mode */
        .endif

        .fnend
        .size   PendSV_Handler, .-PendSV_Handler
#endif

        .end
