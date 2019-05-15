#include <cstdarg>

#include <Arduino.h>
#include <sam.h>

#include "os.h"

#if defined(ARDUINO)

extern "C" {

void os_printf(const char *f, ...) {
    va_list args;
    va_start(args, f);
    char message[128];
    vsnprintf(message, sizeof(message), f, args);
    Serial.print(message);
    va_end(args);
}

bool os_platform_setup() {
    return 1;
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
