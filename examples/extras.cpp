#include <Arduino.h>

#include <os.h>

#define NUMBER_OF_RECEIVERS     (1)
#define NUMBER_OF_SENDERS       (1)

static uint8_t queue[sizeof(os_queue_t) + (4 * 4)];

static os_task_t idle_task;
static uint32_t idle_stack[OSDOTH_STACK_MINIMUM_SIZE_WORDS];

static void task_handler_idle(void *params) {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

static os_task_t sender_tasks[NUMBER_OF_SENDERS];
static uint32_t sender_stacks[NUMBER_OF_SENDERS][256];

static void task_handler_sender(void *params) {
    os_printf("%s started\n", os_task_name());

    while (true) {
        auto tuple = __svc_queue_enqueue((os_queue_t *)queue, (void *)"Jacob", 1000);
        auto status = tuple.status;
        if (status == OSS_SUCCESS) {
            auto wms = random(10, 2000);
            os_printf("%s: success (%s) (%dms)\n", os_task_name(), os_status_str(status), wms);
            __svc_delay(wms);
        }
        else {
            os_printf("%s: fail (%s)\n", os_task_name(), os_status_str(status));
            __svc_delay(100);
        }
    }
}

static os_task_t receiver_tasks[NUMBER_OF_RECEIVERS];
static uint32_t receiver_stacks[NUMBER_OF_RECEIVERS][256];

static void task_handler_receiver(void *params) {
    os_printf("%s started\n", os_task_name());

    __svc_delay(500);

    while (true) {
        auto tuple = __svc_queue_dequeue((os_queue_t *)queue, 1000);
        if (tuple.status == OSS_SUCCESS) {
            auto message = (const char *)tuple.value.ptr;
            auto wms = random(10, 200);
            os_printf("%s: success ('%s') (%dms)\n", os_task_name(), message, wms);
            __svc_delay(wms);
        }
        else {
            os_printf("%s: fail (%s)\n", os_task_name(), os_status_str(tuple.status));
        }
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
    }

    // Call this here because things go horribly if we call from within a task.
    // Something goes south with a malloc.
    random(100, 1000);

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

    for (auto i = 0; i < NUMBER_OF_RECEIVERS; ++i) {
        char temp[32];
        os_snprintf(temp, sizeof(temp), "receiver-%d", i);
        if (!os_task_initialize(&receiver_tasks[i], strdup(temp), OS_TASK_START_RUNNING, &task_handler_receiver, NULL, receiver_stacks[i], sizeof(receiver_stacks[i]))) {
            os_printf("Error: os_task_initialize failed\n");
            os_error(OS_ERROR_APP);
        }
    }

    for (auto i = 0; i < NUMBER_OF_SENDERS; ++i) {
        char temp[32];
        os_snprintf(temp, sizeof(temp), "sender-%d", i);
        if (!os_task_initialize(&sender_tasks[i], strdup(temp), OS_TASK_START_RUNNING, &task_handler_sender, NULL, sender_stacks[i], sizeof(sender_stacks[i]))) {
            os_printf("Error: os_task_initialize failed\n");
            os_error(OS_ERROR_APP);
        }
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
