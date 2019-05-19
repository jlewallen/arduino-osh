#include <cassert>

#include <Arduino.h>
#include <os.h>
#include "printf.h"

static os_task_t tasks[1];
static uint32_t stack1[128];

#if defined(OSH_MTB)
#define DEBUG_MTB_SIZE 256
__attribute__((__aligned__(DEBUG_MTB_SIZE * sizeof(uint32_t)))) uint32_t mtb[DEBUG_MTB_SIZE];
#endif

extern "C" char *sbrk(int32_t i);

static uint32_t free_memory() {
    char stack_dummy = 0;
    return &stack_dummy - sbrk(0);
}

static void task_handler_empty(void *) {
    os_printf("Empty\n");

    while (true) {
        os_printf("Tick\n");
        delay(1000);
    }
}

void setup() {
    Serial.begin(115200);

    while (!Serial && millis() < 2000) {
        delay(10);
    }

    #if defined(OSH_MTB)
    REG_MTB_POSITION = ((uint32_t)(mtb - REG_MTB_BASE)) & 0xFFFFFFF8;
    REG_MTB_FLOW = ((uint32_t)mtb + DEBUG_MTB_SIZE * sizeof(uint32_t)) & 0xFFFFFFF8;
    REG_MTB_MASTER = 0x80000000 + 6;
    #endif

    os_printf("Starting: %d\n", free_memory());

    assert(os_initialize());
    assert(os_task_initialize(&tasks[0], &task_handler_empty, nullptr, stack1, sizeof(stack1)));
    assert(os_start());
}

void loop() {
}

