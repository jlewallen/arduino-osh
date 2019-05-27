/**
 *
 */
#if defined(ARDUINO)

#include "os.h"
#include "printf.h"
#include "internal.h"

#include <Arduino.h>

extern "C" {

static void serial_putchar(char c, void *arg) {
    if (c != 0) {
        Serial.print(c);
        #if defined(OS_CONFIG_DEBUG_RTT)
        SEGGER_RTT_PutChar(0, c);
        #endif
    }
}

uint32_t os_printf(const char *f, ...) {
    va_list args;
    va_start(args, f);
    // __disable_irq();
    auto i = os_vfctprintf(serial_putchar, NULL, f, args);
    va_end(args);
    // __enable_irq();
    return i;
}

os_status_t osi_platform_setup() {
    return OSS_SUCCESS;
}

uint32_t osi_platform_uptime() {
    return millis();
}

uint32_t osi_platform_delay(uint32_t ms) {
    delay(ms);
    return ms;
}

extern void SysTick_DefaultHandler(void);

int32_t sysTickHook(void) {
    SysTick_DefaultHandler();

    osi_irs_systick();

    return 1;
}

}

#endif
