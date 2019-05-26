#include <Arduino.h>

#include <os.h>

static os_task_t idle_task;
static uint32_t idle_stack[OSDOTH_STACK_MINIMUM_SIZE_WORDS];

static void task_handler_idle(void *params) {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

static os_task_t child_task;
static uint32_t child_stack[256];

static void task_handler_child(void *params) {
    os_printf("task started\n");

    auto started = os_uptime();
    while (os_uptime() - started < 5000) {
        __os_svc_delay(100);
    }

    os_printf("task done\n");
}

static os_task_t main_task;
static uint32_t main_stack[256];

static void task_handler_main(void *params) {
    while (true) {
        switch (os_task_get_status(&child_task)) {
        case OS_TASK_STATUS_SUSPENDED: {
            os_task_resume(&child_task);
            break;
        }
        case OS_TASK_STATUS_FINISHED: {
            os_task_start(&child_task);
            break;
        }
        default: {
            break;
        }
        }

        __os_svc_delay(1000);
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
    }

    #if defined(HSRAM_ADDR)
    os_printf("starting: %d (0x%p + %lu) (%lu used)\n", os_free_memory(), HSRAM_ADDR, HSRAM_SIZE, HSRAM_SIZE - os_free_memory());
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

    if (!os_task_initialize(&main_task, "main", OS_TASK_START_RUNNING, &task_handler_main, NULL, main_stack, sizeof(main_stack))) {
        os_printf("Error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&child_task, "child", OS_TASK_START_SUSPENDED, &task_handler_child, NULL, child_stack, sizeof(child_stack))) {
        os_printf("Error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_start()) {
        os_printf("Error: os_start failed\n");
        os_error(OS_ERROR_APP);
    }
}

void loop() {
    while (1);
}
