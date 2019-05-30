/**
 * This software is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this source code. If not, see <http://www.gnu.org/licenses/>.
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

static uint32_t linux_uptime = 0;

uint32_t tests_platform_time(uint32_t time) {
    return linux_uptime = time;
}

uint32_t osi_platform_uptime() {
    return linux_uptime;
}

uint32_t osi_platform_delay(uint32_t ms) {
    return ms;
}

#endif /* ARDUINO */
