/**
 *
 */
#ifndef OS_MUTEX_H
#define OS_MUTEX_H

#if defined(__cplusplus)
extern "C" {
#endif

os_status_t osi_mutex_create(os_mutex_t *mutex);
os_status_t osi_mutex_acquire(os_mutex_t *mutex, uint16_t to);
os_status_t osi_mutex_release(os_mutex_t *mutex);

#if defined(__cplusplus)
}
#endif

#endif
