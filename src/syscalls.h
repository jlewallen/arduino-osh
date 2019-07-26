/**
 * This file is privately included in service.c
 *
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
#ifndef OS_SYSCALLS_H
#define OS_SYSCALLS_H

#include "syscall_plumbing.h"

uint32_t svc_delay(uint32_t ms);
uint32_t svc_block(uint32_t ms, uint32_t flags);
uint32_t svc_printf(const char *str, void *vargs);
uint32_t svc_pstr(const char *str);
uint32_t svc_panic(uint32_t code);
uint32_t svc_abort(uint32_t code);

SVC_1_1(svc_delay, uint32_t, uint32_t, RET_uint32_t);
SVC_2_1(svc_block, uint32_t, uint32_t, uint32_t, RET_uint32_t);
SVC_2_1(svc_printf, uint32_t, const char*, void*, RET_uint32_t);
SVC_1_1(svc_pstr, uint32_t, const char*, RET_uint32_t);
SVC_1_1(svc_panic, uint32_t, uint32_t, RET_uint32_t);
SVC_1_1(svc_abort, uint32_t, uint32_t, RET_uint32_t);

os_status_t svc_queue_create(os_queue_t *queue, os_queue_definition_t *def);
os_tuple_return_type_t svc_queue_enqueue(os_queue_t *queue, void *message, uint32_t to);
os_tuple_return_type_t svc_queue_dequeue(os_queue_t *queue, uint32_t to);

SVC_2_1(svc_queue_create, os_status_t, os_queue_t*, os_queue_definition_t*, RET_os_status_t);
SVC_3_1(svc_queue_enqueue, os_tuple_t, os_queue_t*, void*, uint32_t, RET_os_tuple_t);
SVC_2_1(svc_queue_dequeue, os_tuple_t, os_queue_t*, uint32_t, RET_os_tuple_t);

os_status_t svc_mutex_create(os_mutex_t *mutex, os_mutex_definition_t *def);
os_status_t svc_mutex_acquire(os_mutex_t *mutex, uint32_t to);
os_status_t svc_mutex_release(os_mutex_t *mutex);

SVC_2_1(svc_mutex_create, os_status_t, os_mutex_t*, os_mutex_definition_t*, RET_os_status_t);
SVC_2_1(svc_mutex_acquire, os_status_t, os_mutex_t*, uint32_t, RET_os_status_t);
SVC_1_1(svc_mutex_release, os_status_t, os_mutex_t*, RET_os_status_t);

#endif // OS_SYSCALLS_H
