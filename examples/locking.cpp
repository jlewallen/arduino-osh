#include <Arduino.h>
#include <errno.h>

#include <os.h>

#define NUMBER_OF_TASKS         (4)

static os_mutex_t mutex;
static os_task_t idle_task;
static uint32_t idle_stack[OSDOTH_STACK_MINIMUM_SIZE_WORDS];
static os_task_t tasks[NUMBER_OF_TASKS];

static void task_handler_idle(void *params) {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

static void task_handler(void *params) {
    os_printf("%s started\n", os_task_name());

    while (true) {
        auto started = os_uptime();
        auto status = __svc_mutex_acquire(&mutex, 500);
        if (status == OSS_SUCCESS) {
            auto elapsed = os_uptime() - started;
            os_printf("%s acquire (%s) (%dms)\n", os_task_name(), os_status_str(status), elapsed);

            auto wms = random(100, 1000);
            __svc_delay(wms);

            os_printf("%s releasing\n", os_task_name());
            OSDOTH_ASSERT(__svc_mutex_release(&mutex) == OSS_SUCCESS);
        }
        else {
            auto elapsed = os_uptime() - started;
            os_printf("%s failed (%s) (after %lums)\n", os_task_name(), os_status_str(status), elapsed);
            __svc_delay(10);
        }
    }
}

void setup() {
    uint32_t stacks[NUMBER_OF_TASKS][256];

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

    for (auto i = 0; i < NUMBER_OF_TASKS; ++i) {
        char temp[32];
        os_snprintf(temp, sizeof(temp), "task-%d", i);
        if (!os_task_initialize(&tasks[i], strdup(temp), OS_TASK_START_RUNNING, &task_handler, NULL, stacks[i], sizeof(stacks[i]))) {
            os_printf("Error: os_task_initialize failed\n");
            os_error(OS_ERROR_APP);
        }
    }

    os_mutex_create(&mutex);

    if (!os_start()) {
        os_printf("Error: os_start failed\n");
        os_error(OS_ERROR_APP);
    }
}

void loop() {
    while (1);
}
