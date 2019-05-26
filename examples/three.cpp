#include <Arduino.h>

#include <os.h>

static void task_handler(void *arg) {
    auto time = (uint32_t)arg;
    auto lastTick = (uint32_t)0;
    auto now = (uint32_t)0;

    while (true) {
        __disable_irq();
        os_printf("Waiting %lu\n", now - lastTick, os_free_memory());
        Serial.flush();
        __enable_irq();

        lastTick = os_uptime();

        __os_svc_delay(time);

        now = os_uptime();
    }
}

static os_task_t idle_task;

static uint32_t idle_stack[OSDOTH_STACK_MINIMUM_SIZE_WORDS];

static void task_handler_idle(void *params) {
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

static os_task_t tasks[3];
static uint32_t stack1[256];
static uint32_t stack2[256];
static uint32_t stack3[256];

void setup() {
    Serial.begin(115200);
    #if defined(__SAMD21__)
    Serial5.begin(115200);
    #endif
    while (!Serial && millis() < 2000) {
    }

    uint32_t p1 = 4000;
    uint32_t p2 = p1 / 2;
    uint32_t p3 = p1 / 4;

    #if defined(HSRAM_ADDR)
    os_printf("starting: %d (0x%p + %lu) (%lu used)\n", os_free_memory(), HSRAM_ADDR, HSRAM_SIZE, HSRAM_SIZE - os_free_memory());
    #else
    os_printf("starting: %d\n", os_free_memory());
    #endif

    if (!os_initialize()) {
        os_printf("error: os_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&idle_task, "idle", OS_TASK_START_RUNNING, &task_handler_idle, NULL, idle_stack, sizeof(idle_stack))) {
        os_printf("error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&tasks[0], "task1", OS_TASK_START_RUNNING, &task_handler, (void*)p1, stack1, sizeof(stack1))) {
        os_printf("error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&tasks[1], "task2", OS_TASK_START_RUNNING, &task_handler, (void*)p2, stack2, sizeof(stack2))) {
        os_printf("error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_task_initialize(&tasks[2], "task3", OS_TASK_START_RUNNING, &task_handler, (void*)p3, stack3, sizeof(stack3))) {
        os_printf("error: os_task_initialize failed\n");
        os_error(OS_ERROR_APP);
    }

    if (!os_start()) {
        os_printf("error: os_start failed\n");
        os_error(OS_ERROR_APP);
    }
}

void loop() {
    while (1);
}
