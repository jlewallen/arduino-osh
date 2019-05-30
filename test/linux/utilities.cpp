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
#include "utilities.h"

void PrintTo(os_task_t *task, std::ostream* os) {
    if (task == NULL) {
        *os << "<null>";
    }
    else {
        *os << "T<'" << task->name << "'>";
    }
}

extern "C" {

void osi_assert(const char *assertion, const char *file, int line) {
    FAIL() << "Assertion \"" << assertion << "\" failed. File: " << file << " Line: " << line;
}

}
