/**
 *
 */
#ifndef OS_SERVICE_H
#define OS_SERVICE_H

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
uint32_t os_example();

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
os_status_t os_queue_create(os_queue_t *queue, uint32_t size);

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
os_status_t os_mutex_create(os_mutex_t *mutex);

/**
 *
 */
os_status_t os_mutex_acquire(os_mutex_t *mutex, uint32_t to);

/**
 *
 */
os_status_t os_mutex_release(os_mutex_t *mutex);

#if defined(__cplusplus)
}
#endif

#endif
