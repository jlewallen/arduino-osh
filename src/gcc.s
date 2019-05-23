.file   "gcc.s"
.syntax unified
.thumb

.equ    OS_TASK_STACK_KIND, 12
.equ    OS_TASK_SP, 0

.equ    OSG_RUNNING, 0
.equ    OSG_SCHEDULED, 4

.section ".text"
.align  2

#if defined(__SAMD21__)
#include "arch/m0.s"
#endif

#if defined(__SAMD51__)
#include "arch/m4.s"
#endif

.end
