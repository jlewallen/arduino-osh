#include <cassert>

#include <Arduino.h>
#include <os.h>
#include "printf.h"

static os_task_t tasks[1];
static uint32_t stack1[128] = { 0 };

static os_task_t idle_task;
static uint32_t idle_stack[OSDOTH_STACK_MINIMUM_SIZE_WORDS];

static void task_handler_empty(void *) {
    os_printf("Empty\n");
    os_delay(100);

    #if defined(TEST_HARD_FAULT_1)
    *((uint32_t *)(0xdeadbeef)) = 1;
    #endif

    pinMode(LED_BUILTIN, OUTPUT);

    while (true) {
        os_printf("Tick\n");
        digitalWrite(LED_BUILTIN, LOW);
        os_delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        os_delay(500);
    }
}

static void task_handler_idle(void *params) {
    while (true) {
        os_delay(1000);
    }
}

void setup() {
    Serial.begin(115200);

    while (!Serial && millis() < 2000) {
        os_delay(10);
    }

    #if defined(HSRAM_ADDR)
    os_printf("Starting: %d (0x%p + %lu)\n", os_free_memory(), HSRAM_ADDR, HSRAM_SIZE);
    #else
    os_printf("Starting: %d\n", os_free_memory());
    #endif

    assert(os_initialize());
    assert(os_task_initialize(&idle_task, "idle", OS_TASK_START_RUNNING, &task_handler_idle, NULL, idle_stack, sizeof(idle_stack)));
    assert(os_task_initialize(&tasks[0], "simple", OS_TASK_START_RUNNING, &task_handler_empty, nullptr, stack1, sizeof(stack1)));
    assert(os_start());
}

void loop() {
}

