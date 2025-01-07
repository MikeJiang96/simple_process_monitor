/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

#ifndef SIMPLE_PROCESS_MONITOR_SYSTEM_INFO_H
#define SIMPLE_PROCESS_MONITOR_SYSTEM_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/time.h>
#include <sys/utsname.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CpuUsageTime {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long hardirq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guest_nice;
    unsigned long long total;
} CpuUsageTime;

/** Defines data for systemwide statistic */
typedef struct SystemInfo_T {
    struct {
        struct {
            CpuUsageTime old;

            float user;       /**< Time in user space [%] */
            float nice;       /**< Time in user space with low priority [%] */
            float system;     /**< Time in kernel space [%] */
            float idle;       /**< Idle time [%] */
            float iowait;     /**< Idle time while waiting for I/O [%] */
            float hardirq;    /**< Time servicing hardware interrupts [%] */
            float softirq;    /**< Time servicing software interrupts [%] */
            float steal;      /**< Stolen time, which is the time spent in other operating systems when running in a
                                 virtualized environment [%] */
            float guest;      /**< Time spent running a virtual CPU for guest operating systems under the control of the
                                 kernel [%] */
            float guest_nice; /**< Time spent running a niced guest (virtual CPU for guest operating systems under the
                                 control of the kernel) [%] */
        } usage;
    } cpu;

    struct {
        struct {
            float percent;            /**< Total real memory in use in the system */
            unsigned long long bytes; /**< Total real memory in use in the system */
        } usage;
    } memory;

    struct {
        unsigned long long size; /**< Swap size */

        struct {
            float percent;            /**< Total swap in use in the system */
            unsigned long long bytes; /**< Total swap in use in the system */
        } usage;
    } swap;

    struct {
        long long allocated; /**< Number of allocated filedescriptors */
        long long unused;    /**< Number of unused filedescriptors */
        long long maximum;   /**< Filedescriptors limit */
    } filedescriptors;

    size_t argmax;            /**< Program arguments maximum [B] */
    double loadavg[3];        /**< Load average triple */
    struct utsname uname;     /**< Platform information provided by uname() */
    struct timeval collected; /**< When were data collected */
} SystemInfo_T;

struct FixedSystemInfo {
    long page_size;
    double hz;
    int cpu_count;
    unsigned long long memory_size;
};

extern struct FixedSystemInfo g_fixed_system_info;

/**
 * Initialize the system information
 * @return true if succeeded otherwise false.
 */
bool init_system_info(SystemInfo_T *si);

/**
 * Update system statistic
 * @return true if successful, otherwise false
 */
bool update_system_info(SystemInfo_T *si);

unsigned long long getNowSingleCoreCpuTime(void);

#ifdef __cplusplus
}
#endif

#endif
