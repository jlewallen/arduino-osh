#include <Arduino.h>
#include <sam.h>

#include "os.h"

#if defined(ARDUINO)

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

void HardFault_Handler() {
    os_irs_hard_fault();
}

void PendSV_Handler() {
    os_irs_pendv();
}

#endif
