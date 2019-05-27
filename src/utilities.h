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
#ifndef OS_UTILITIES_H
#define OS_UTILITIES_H

#if defined(__cplusplus)
extern "C" {
#endif

static inline void blocked_append(os_blocked_t *blocked, os_task_t *task) {
    OS_ASSERT(task->nblocked == NULL);
    if (blocked->tasks == NULL) {
        blocked->tasks = task;
    }
    else {
        for (os_task_t *iter = blocked->tasks; ; iter = iter->nblocked) {
            OS_ASSERT(iter != task);
            if (iter->nblocked == NULL) {
                iter->nblocked = task;
                break;
            }
        }
    }
}

#if defined(__cplusplus)
}
#endif

#endif // OS_UTILITIES_H
