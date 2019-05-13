#include <cassert>

#include <Arduino.h>
#include <os.h>

static os_task_t tasks[1];
static uint32_t stack1[128];

#define DEBUG_MTB_SIZE 256
__attribute__((__aligned__(DEBUG_MTB_SIZE * sizeof(uint32_t)))) uint32_t mtb[DEBUG_MTB_SIZE];

void task_handler_empty(void *) {
    Serial.println("Empty");

    while (true) {
        Serial.println("-");
        delay(1000);
    }
}

void setup() {
    Serial.begin(115200);

    while (!Serial && millis() < 2000) {
    }

    REG_MTB_POSITION = ((uint32_t)(mtb - REG_MTB_BASE)) & 0xFFFFFFF8;
    REG_MTB_FLOW = ((uint32_t)mtb + DEBUG_MTB_SIZE * sizeof(uint32_t)) & 0xFFFFFFF8;
    REG_MTB_MASTER = 0x80000000 + 6;

    Serial.println("Starting");

    assert(os_initialize());
    assert(os_task_initialize(&tasks[0], &task_handler_empty, nullptr, stack1, sizeof(stack1)));
    assert(os_start());

    REG_MTB_MASTER = 0x00000000;
    volatile uint32_t i = 0;
    while (true) {
        i++;
    }
}

void loop() {
}

