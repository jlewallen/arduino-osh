  .file   "gcc.s"
  .syntax unified

  .equ    TCB_STACKF, 12
  .equ    TCB_TSTACK, 0

  .thumb

  .section ".text"
  .align  2

#if defined(__SAMD51__)
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

        mrs      r12, psp                   /* Read PSP */
        tst      lr, #0x10                  /* is it extended frame? */
        itte     eq
        vstmdbeq r12!, {s16 - s31}          /* yes, stack also VFP hi-regs */
        moveq    r0, #0x01                  /* os_tsk->stack_frame val */
        movne    r0, #0x00
        strb     r0, [r1, #TCB_STACKF]      /* os_tsk.run->stack_frame = val */
        stmdb    r12!, {r4 - r11}           /* Save Old context */
        str      r12, [r1, #TCB_TSTACK]     /* Update os_tsk.run->tsk_stack */

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

        ldr      r12, [r3, #TCB_TSTACK]     /* os_tsk.next->tsk_stack */
        ldmia    r12!, {r4 - r11}           /* Restore New Context */
        ldrb     r0, [r3, #TCB_STACKF]      /* Stack Frame */
        cmp      r0, #0                     /* Basic/Extended Stack Frame */
        itee     eq
        mvneq    lr, #~0xFFFFFFFD           /* set EXC_RETURN value */
        mvnne    lr, #~0xFFFFFFED
        vldmiane r12!, {s16 - s31}          /* restore VFP hi-registers */
        msr      psp, r12                   /* Write PSP */

pendsv_done:
        .ifdef   IFX_XMC4XXX
        push     {lr}
        pop      {pc}
        .else
        bx       lr                         /* Return to Thread Mode */
        .endif

        .fnend
        .size   PendSV_Handler, .-PendSV_Handler

        .end
#endif
