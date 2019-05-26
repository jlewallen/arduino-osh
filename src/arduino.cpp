#include <cstdarg>

#include <Arduino.h>
#include <sam.h>

#include "os.h"
#include "printf.h"

#if defined(ARDUINO)

extern "C" {

static void serial_putchar(char c, void *arg) {
    if (c != 0) {
        Serial.print(c);
    }
}

void os_printf(const char *f, ...) {
    va_list args;
    va_start(args, f);
    os_vfctprintf(serial_putchar, NULL, f, args);
    va_end(args);
}

bool os_platform_setup() {
    return 1;
}

void os_platform_led(bool on) {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
}

uint32_t os_platform_uptime() {
    return millis();
}

void os_platform_delay(uint32_t ms) {
    delay(ms);
}

int sysTickHook(void);

extern void SysTick_DefaultHandler(void);

int sysTickHook(void) {
    SysTick_DefaultHandler();

    os_irs_systick();

    return 1;
}

}

#endif
