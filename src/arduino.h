/**
 *
 */
#ifndef OS_ARDUINO_H
#define OS_ARDUINO_H

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 */
os_status_t osi_platform_setup();

/**
 *
 */
uint32_t osi_platform_uptime();

/**
 *
 */
uint32_t osi_platform_delay(uint32_t ms);

#if defined(__cplusplus)
}
#endif

#endif /* OS_ARDUINO_H */
