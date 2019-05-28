/**
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
#ifndef OS_SERVICE_H
#define OS_SERVICE_H

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
uint32_t os_delay(uint32_t ms);

/**
 *
 */
uint32_t os_block(uint32_t ms, uint32_t flags);

/**
 *
 */
uint32_t os_pstr(const char *str);

/**
 *
 */
uint32_t os_example();

/**
 *
 */
os_status_t os_queue_create(os_queue_t *queue, os_queue_definition_t *def);

/**
 *
 */
os_tuple_t os_queue_enqueue(os_queue_t *queue, void *message, uint32_t to);

/**
 *
 */
os_tuple_t os_queue_dequeue(os_queue_t *queue, uint32_t to);

/**
 *
 */
os_status_t os_mutex_create(os_mutex_t *mutex, os_mutex_definition_t *def);

/**
 *
 */
os_status_t os_mutex_acquire(os_mutex_t *mutex, uint32_t to);

/**
 *
 */
os_status_t os_mutex_release(os_mutex_t *mutex);

/**
 *
 */
#define os_queue_define(name, size)                                              \
    os_queue_definition_t _os_queue_def_##name = { #name, size };                \
    uint32_t _os_queue_##name[(sizeof(os_queue_t) / sizeof(uint32_t)) + (size)];

/**
 *
 */
#define os_queue(name)                (os_queue_t *)_os_queue_##name

/**
 *
 */
#define os_queue_def(name)            (os_queue_definition_t *)&_os_queue_def_##name

/**
 *
 */
#define os_mutex_define(name)                               \
    os_mutex_definition_t _os_mutex_def_##name = { #name }; \
    os_mutex_t _os_mutex_##name;

/**
 *
 */
#define os_mutex(name)                (os_mutex_t *)&_os_mutex_##name

/**
 *
 */
#define os_mutex_def(name)            (os_mutex_definition_t *)&_os_mutex_def_##name

#if defined(__cplusplus)
}
#endif

#endif
