#ifndef OS_QUEUE_H
#define OS_QUEUE_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct os_queue_t {
    uint16_t size;
    uint16_t number;
    uint16_t first;
    uint16_t last;
    void *messages[1];
} os_queue_t;

os_status_t os_queue_create(os_queue_t *queue, uint16_t size);
os_status_t os_queue_enqueue(os_queue_t *queue, void *message, uint16_t to);
os_status_t os_queue_dequeue(os_queue_t *queue, void **message, uint16_t to);

#if defined(__cplusplus)
}
#endif

#endif
