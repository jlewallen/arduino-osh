#include <Arduino.h>

#include "os.h"

static void task_handler(void *arg) {
    uint32_t time = (uint32_t)arg;
    uint32_t lastTick = 0;
    uint32_t now = 0;

    while (true) {
        // __disable_irq();
        Serial.print("Waiting ");
        Serial.print(time);
        Serial.print(" ");
        Serial.print(now - lastTick);
        Serial.println("");
        lastTick = millis();
        // __enable_irq();

        delay(time);

        now = millis();
    }
}

void setup() {
    Serial.begin(115200);
    #if defined(__SAMD21__)
    Serial5.begin(115200);
    #endif
    while (!Serial && millis() < 2000) {
    }

    Serial.println("Hello, world");

    static uint32_t stack1[128];
    static uint32_t stack2[128];
    static uint32_t stack3[128];

    uint32_t p1 = 4000;
    uint32_t p2 = p1/2;
    uint32_t p3 = p1/4;

    auto status = os_initialize();
    if (!status) {
        Serial.println("Error: os_initialize failed");
        while (true);
    }

    status = os_task_initialize(&task_handler, (void*)p1, stack1, sizeof(stack1));
    if (!status) {
        Serial.println("Error: os_task_initialize failed");
        while (true);
    }

    status = os_task_initialize(&task_handler, (void*)p2, stack2, sizeof(stack2));
    if (!status) {
        Serial.println("Error: os_task_initialize failed");
        while (true);
    }

    status = os_task_initialize(&task_handler, (void*)p3, stack3, sizeof(stack3));
    if (!status) {
        Serial.println("Error: os_task_initialize failed");
        while (true);
    }

    status = os_start();
    if (!status) {
        Serial.println("Error: os_start failed");
        while (true);
    }

    while (1);
}

void loop() {
}
