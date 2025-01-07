/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.
 */

#include "file.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "Str.h"
#include "debug.h"

bool file_readProc(char *buf, int buf_size, const char *name, int pid, int tid, int *bytes_read) {
    assert(buf);
    assert(name);

    char filename[STRLEN];
    if (pid < 0)
        snprintf(filename, sizeof(filename), "/proc/%s", name);
    else if (tid < 0)
        snprintf(filename, sizeof(filename), "/proc/%d/%s", pid, name);
    else {
        snprintf(filename, sizeof(filename), "/proc/%d/task/%d/%s", pid, tid, name);
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        DEBUG("Cannot open proc file '%s' -- %s\n", filename, STRERROR);
        return false;
    }

    bool rv = false;
    int bytes = (int)read(fd, buf, buf_size - 1);
    if (bytes >= 0) {
        if (bytes_read)
            *bytes_read = bytes;
        buf[bytes] = 0;
        rv = true;
    } else {
        *buf = 0;
        DEBUG("Cannot read proc file '%s' -- %s\n", filename, STRERROR);
    }

    if (close(fd) < 0)
        Log_error("Failed to close proc file '%s' -- %s\n", filename, STRERROR);

    return rv;
}
