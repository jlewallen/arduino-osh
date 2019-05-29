#include <Arduino.h>
#include <os.h>
#include <malloc.h>

#define NUMBER_OF_RECEIVERS            (4)
#define RECEIVER_PROCESSING_MINIMUM    (10)
#define RECEIVER_PROCESSING_MAXIMUM    (500)

#define NUMBER_OF_SENDERS              (6)
#define SENDER_DELAY_MINIMUM           (10)
#define SENDER_DELAY_MAXIMUM           (200)

static os_task_t idle_task;
static uint32_t idle_stack[OS_STACK_MINIMUM_SIZE_WORDS];
static os_task_t sender_tasks[NUMBER_OF_SENDERS];
static os_task_t receiver_tasks[NUMBER_OF_RECEIVERS];

os_queue_define(queue, 4);

static const char *os_pstrdup(const char *f, ...) {
    char message[64];
    va_list args;
    va_start(args, f);
    os_vsnprintf(message, sizeof(message), f, args);
    va_end(args);
    auto copy = strdup(message);
    OS_ASSERT(copy != NULL);
    return copy;
}

static void task_handler_idle(void *params) {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

static void task_handler_sender(void *params) {
    os_printf("%s started (%d)\n", os_task_name(), __get_CONTROL());

    uint32_t counter = 0;
    uint32_t memory_reported = os_uptime();

    while (true) {
        auto started = os_uptime();
        auto message = (char *)os_pstrdup("message<%d>", counter++);
        auto tuple = os_queue_enqueue(os_queue(queue), message, 1000);
        auto status = tuple.status;
        if (status == OSS_SUCCESS) {
            auto wms = random(SENDER_DELAY_MINIMUM, SENDER_DELAY_MAXIMUM);
            os_printf("%s: success (%dms)\n", os_task_name(), wms);
            os_delay(wms);
        }
        else {
            auto elapsed = os_uptime() - started;
            os_printf("%s: fail (%s) (after %lums)\n", os_task_name(), os_status_str(status), elapsed);
            os_delay(100);
        }

        if (os_uptime() - memory_reported > 10000) {
            auto mi = mallinfo();
            os_printf("memory: arena=%lu uordblks=%lu\n", mi.arena, mi.uordblks);
            memory_reported = os_uptime();
        }
    }
}

static void task_handler_receiver(void *params) {
    os_printf("%s started (%d)\n", os_task_name(), __get_CONTROL());

    os_delay(500);

    while (true) {
        auto started = os_uptime();
        auto tuple = os_queue_dequeue(os_queue(queue), 1000);
        if (tuple.status == OSS_SUCCESS) {
            auto message = (const char *)tuple.value.ptr;
            auto wms = random(RECEIVER_PROCESSING_MINIMUM, RECEIVER_PROCESSING_MAXIMUM);
            os_printf("%s: success ('%s') (%dms)\n", os_task_name(), message, wms);
            free((void *)message);
            os_delay(wms);
        }
        else {
            auto elapsed = os_uptime() - started;
            os_printf("%s: fail (%s) (after %lums)\n", os_task_name(), os_status_str(tuple.status), elapsed);
        }
    }
}

void setup() {
    uint32_t sender_stacks[NUMBER_OF_SENDERS][256];
    uint32_t receiver_stacks[NUMBER_OF_RECEIVERS][256];

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

    OS_CHECK(os_initialize());

    OS_CHECK(os_task_initialize(&idle_task, "idle", OS_TASK_START_RUNNING, &task_handler_idle, NULL, idle_stack, sizeof(idle_stack)));

    for (auto i = 0; i < NUMBER_OF_RECEIVERS; ++i) {
        char temp[32];
        os_snprintf(temp, sizeof(temp), "receiver-%d", i);
        OS_CHECK(os_task_initialize(&receiver_tasks[i], strdup(temp), OS_TASK_START_RUNNING, &task_handler_receiver, NULL, receiver_stacks[i], sizeof(receiver_stacks[i])));
    }

    for (auto i = 0; i < NUMBER_OF_SENDERS; ++i) {
        char temp[32];
        os_snprintf(temp, sizeof(temp), "sender-%d", i);
        OS_CHECK(os_task_initialize(&sender_tasks[i], strdup(temp), OS_TASK_START_RUNNING, &task_handler_sender, NULL, sender_stacks[i], sizeof(sender_stacks[i])));
    }

    OS_CHECK(os_queue_create(os_queue(queue), os_queue_def(queue)));

    OS_CHECK(os_start());
}

void loop() {
    while (1);
}
