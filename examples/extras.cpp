#include <Arduino.h>

#include <os.h>

static uint8_t queue[sizeof(os_queue_t) + (4 * 4)];

static os_task_t idle_task;
static uint32_t idle_stack[OSDOTH_STACK_MINIMUM_SIZE_WORDS];

static void task_handler_idle(void *params) {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

static os_task_t sender_task;
static uint32_t sender_stack[256];

static void task_handler_sender(void *params) {
    os_printf("sender started\n");

    while (true) {
        auto status = __svc_queue_enqueue((os_queue_t *)queue, (void *)"Jacob", 1000);
        if (status == OSS_SUCCESS) {
            os_printf("send: success (%s)\n", os_status_str(status));
        }
        else {
            __os_svc_delay(1000);
            os_printf("send: fail (%s)\n", os_status_str(status));
        }
    }
}

static os_task_t receiver_task;
static uint32_t receiver_stack[256];

static void task_handler_receiver(void *params) {
    os_printf("receiver started\n");

    __os_svc_delay(500);

    while (true) {
        char *message = NULL;
        auto status = __svc_queue_dequeue((os_queue_t *)queue, (void **)&message, 1000);
        if (status == OSS_SUCCESS) {
            OSDOTH_ASSERT(message != NULL);
            os_printf("receiver: success ('%s')\n", message);
        }
        else {
            __os_svc_delay(1000);
            os_printf("receiver: fail (%s)\n", os_status_str(status));
        }
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
    }

    #if defined(HSRAM_ADDR)
    os_printf("starting: %d (0x%p + %lu) (%lu used) (%d)\n", os_free_memory(), HSRAM_ADDR, HSRAM_SIZE, HSRAM_SIZE - os_free_memory(), __get_CONTROL());
    #else
    os_printf("starting: %d\n", os_free_memory());
    #endif

    if (!os_initialize()) {
        os_printf("Error: os_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&idle_task, "idle", OS_TASK_START_RUNNING, &task_handler_idle, NULL, idle_stack, sizeof(idle_stack))) {
        os_printf("Error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&receiver_task, "receiver", OS_TASK_START_RUNNING, &task_handler_receiver, NULL, receiver_stack, sizeof(receiver_stack))) {
        os_printf("Error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&sender_task, "sender", OS_TASK_START_RUNNING, &task_handler_sender, NULL, sender_stack, sizeof(sender_stack))) {
        os_printf("Error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    os_queue_create((os_queue_t *)queue, 4);

    if (!os_start()) {
        os_printf("Error: os_start failed\n");
        os_error(OS_ERROR_APP);
    }
}

void loop() {
    while (1);
}
