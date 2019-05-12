#include <Arduino.h>
#include <os.h>

void setup() {
    Serial.begin(115200);

    while (!Serial && millis() < 10000) {
    }

    Serial.println("Starting");
}

void loop() {
    delay(10);
}

