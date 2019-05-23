/*
 * This file is part of os.h.
 */

#ifndef OS_ARDUINO_H
#define OS_ARDUINO_H

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
#define OS_DECLARE_PENDSV_HANDLER()               void PendSV_Handler()

/**
 *
 */
#define OS_DECLARE_HARD_FAULT_HANDLER()           void HardFault_Handler()

/**
 *
 */
#define OS_DECLARE_USAGE_FAULT_HANDLER()          void UsageFault_Handler()

/**
 *
 */
bool os_platform_setup();

/**
 *
 */
void os_platform_led(bool on);

/**
 *
 */
void os_platform_delay(uint32_t ms);

#if defined(__cplusplus)
}
#endif

#endif /* OS_ARDUINO_H */
