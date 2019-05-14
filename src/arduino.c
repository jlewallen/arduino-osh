#include <Arduino.h>

#include "os.h"

#if defined(ARDUINO)

extern void SysTick_DefaultHandler(void);

int sysTickHook() {
    SysTick_DefaultHandler();

    os_systick();

    return 1;
}

#endif
