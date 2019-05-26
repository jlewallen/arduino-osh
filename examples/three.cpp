#include <Arduino.h>

#include "os.h"


static void task_handler(void *arg) {
    auto time = (uint32_t)arg;
    auto lastTick = (uint32_t)0;
    auto now = (uint32_t)0;
    auto faulted = false;

    while (true) {
        __disable_irq();
        Serial.print("Waiting ");
        Serial.print(time);
        Serial.print(" ");
        Serial.print(now - lastTick);
        Serial.println("");
        __enable_irq();

        lastTick = millis();

        __os_svc_delay(time);

        now = millis();
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
static uint32_t stack1[64];
static uint32_t stack2[64];
static uint32_t stack3[64];

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

    Serial.println("Starting");

    auto status = os_initialize();
    if (!status) {
        Serial.println("Error: os_initialize failed");
        while (true);
    }

    status = os_task_initialize(&idle_task, "idle", OS_TASK_START_RUNNING, &task_handler_idle, NULL, idle_stack, sizeof(idle_stack));
    if (!status) {
        Serial.println("Error: os_task_initialize failed");
        while (true);
    }

    status = os_task_initialize(&tasks[0], "task1", OS_TASK_START_RUNNING, &task_handler, (void*)p1, stack1, sizeof(stack1));
    if (!status) {
        Serial.println("Error: os_task_initialize failed");
        while (true);
    }

    status = os_task_initialize(&tasks[1], "task2", OS_TASK_START_RUNNING, &task_handler, (void*)p2, stack2, sizeof(stack2));
    if (!status) {
        Serial.println("Error: os_task_initialize failed");
        while (true);
    }

    status = os_task_initialize(&tasks[2], "task3", OS_TASK_START_RUNNING, &task_handler, (void*)p3, stack3, sizeof(stack3));
    if (!status) {
        Serial.println("Error: os_task_initialize failed");
        while (true);
    }

    status = os_start();
    if (!status) {
        Serial.println("Error: os_start failed");
        while (true);
    }
}

void loop() {
    while (1);
}
