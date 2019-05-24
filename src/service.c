#include "os.h"

uint32_t os_svc_example() {
    os_printf("os_svc_example\n");
    return 0;
}

uint32_t os_svc_delay(uint32_t ms) {
    OSDOTH_ASSERT(osg.running != NULL);
    OSDOTH_ASSERT(osg.scheduled != osg.running);

    // osg.running->status = OS_TASK_STATUS_WAIT;
    osg.running->delay = os_uptime() + ms;

    if (osg.scheduled == NULL) {
        os_schedule();
    }

    OSDOTH_ASSERT(osg.scheduled != NULL);
    OSDOTH_ASSERT(osg.scheduled != osg.running);

    return 0;
}
