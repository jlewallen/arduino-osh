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

enum os_task_status {
    OS_TASK_STATUS_IDLE = 1,
    OS_TASK_STATUS_ACTIVE,
};

typedef struct os_task_t {
    /* The stack pointer (sp) has to be the first element as it is located
       at the same address as the structure itself (which makes it possible
       to locate it safely from assembly implementation of PendSV_Handler).
       The compiler might add padding between other structure elements. */
    volatile uint32_t sp;
    volatile enum os_task_status status;
    void (*handler)(void *params);
    void *params;
    struct os_task_t *np;
} os_task_t;

bool os_initialize();

bool os_task_initialize(os_task_t *task, void (*handler)(void *params), void *params, uint32_t *stack, size_t stack_size);

bool os_start();

#endif /* OS_H */
