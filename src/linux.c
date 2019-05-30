/**
 *
 */
#if !defined(ARDUINO)

#include <stdio.h>

#include "os.h"
#include "printf.h"
#include "internal.h"

uint32_t os_printf(const char *f, ...) {
    va_list args;
    va_start(args, f);
    vfprintf(stderr, f, args);
    va_end(args);
    return 0;
}

os_status_t osi_platform_setup() {
    return OSS_SUCCESS;
}

uint32_t osi_platform_uptime() {
    return 0;
}

uint32_t osi_platform_delay(uint32_t ms) {
    return ms;
}

#endif
