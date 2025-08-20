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

#ifndef UTIL_TIME_H
#define UTIL_TIME_H

#include <stddef.h>
#include <sys/time.h>

/**
 * Returns the time since the epoch measured in milliseconds.
 * @return A 64 bits long representing the systems notion of milliseconds
 * since the <strong>epoch</strong> (January 1, 1970, 00:00:00 GMT) in
 * Coordinated Universal Time (UTC).
 */
static inline long long Time_milli(void) {
    struct timeval t;

    (void)gettimeofday(&t, NULL);

    return (long long)t.tv_sec * 1000 + (long long)t.tv_usec / 1000;
}

/**
 * Returns the time since the epoch measured in seconds.
 * @return A time_t representing the systems notion of seconds since the
 * <strong>epoch</strong> (January 1, 1970, 00:00:00 GMT) in Coordinated
 * Universal Time (UTC).
 */
static inline time_t Time_now(void) {
    struct timeval t;

    (void)gettimeofday(&t, NULL);

    return t.tv_sec;
}

#endif
