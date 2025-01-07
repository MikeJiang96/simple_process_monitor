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

#include "simple_process_monitor/system_info.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "util/Str.h"
#include "util/debug.h"
#include "util/file.h"

struct FixedSystemInfo g_fixed_system_info;

static bool _init_fixed_system_info(struct FixedSystemInfo *pInfo) {
    if ((pInfo->hz = sysconf(_SC_CLK_TCK)) <= 0.) {
        DEBUG("system statistic error -- cannot get hz: %s\n", STRERROR);
        return false;
    }

    if ((pInfo->page_size = sysconf(_SC_PAGESIZE)) <= 0) {
        DEBUG("system statistic error -- cannot get page size: %s\n", STRERROR);
        return false;
    }

    if ((pInfo->cpu_count = sysconf(_SC_NPROCESSORS_CONF)) < 0) {
        DEBUG("system statistic error -- cannot get cpu count: %s\n", STRERROR);
        return false;
    } else if (pInfo->cpu_count == 0) {
        DEBUG("system reports cpu count 0, setting dummy cpu count 1\n");
        pInfo->cpu_count = 1;
    }

    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[STRLEN];
        pInfo->memory_size = 0L;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "MemTotal: %llu", &pInfo->memory_size) == 1) {
                pInfo->memory_size *= 1024;
                break;
            }
        }
        fclose(f);
        if (!pInfo->memory_size)
            DEBUG("system statistic error -- cannot get real memory amount\n");
    } else {
        DEBUG("system statistic error -- cannot open /proc/meminfo\n");
    }

    return true;
}

static void __attribute__((constructor)) _constructor(void) {
    assert(_init_fixed_system_info(&g_fixed_system_info));
}

/**
 * This routine returns 'nelem' double precision floats containing
 * the load averages in 'loadv'; at most 3 values will be returned.
 * @param loadv destination of the load averages
 * @param nelem number of averages
 * @return: 0 if successful, -1 if failed (and all load averages are 0).
 */
static int getloadavg_sysdep(double *loadv, int nelem) {
#ifdef HAVE_GETLOADAVG
    return getloadavg(loadv, nelem);
#else
    char buf[STRLEN];
    double load[3];
    if (!file_readProc(buf, sizeof(buf), "loadavg", -1, -1, NULL))
        return -1;
    if (sscanf(buf, "%lf %lf %lf", &load[0], &load[1], &load[2]) != 3) {
        DEBUG("system statistic error -- cannot get load average\n");
        return -1;
    }
    for (int i = 0; i < nelem; i++)
        loadv[i] = load[i];
    return 0;
#endif
}

/**
 * This routine returns real memory in use.
 * @return: true if successful, false if failed
 */
static bool used_system_memory_sysdep(SystemInfo_T *si) {
    char *ptr;
    char buf[2048];
    unsigned long long mem_total = 0ULL;
    unsigned long long mem_available = 0ULL;
    unsigned long long mem_free = 0ULL;
    unsigned long long buffers = 0ULL;
    unsigned long long cached = 0ULL;
    unsigned long long slabreclaimable = 0ULL;
    unsigned long long swap_total = 0ULL;
    unsigned long long swap_free = 0ULL;
    unsigned long long zfsarcsize = 0ULL;

    if (!file_readProc(buf, sizeof(buf), "meminfo", -1, -1, NULL)) {
        Log_error("system statistic error -- cannot get system memory info\n");
        goto error;
    }

    // Update memory total (physical memory can be added to the online system on some machines, also LXC/KVM containers
    // MemTotal is dynamic and changes frequently
    if ((ptr = strstr(buf, "MemTotal:")) && sscanf(ptr + 9, "%llu", &mem_total) == 1) {
        g_fixed_system_info.memory_size = mem_total * 1024;
    }

    // Check if the "MemAvailable" value is available on this system. If it is, we will use it. Otherwise we will
    // attempt to calculate the amount of available memory ourself
    if ((ptr = strstr(buf, "MemAvailable:")) && sscanf(ptr + 13, "%llu", &mem_available) == 1) {
        si->memory.usage.bytes = g_fixed_system_info.memory_size - mem_available * 1024;
    } else {
        DEBUG(
            "'MemAvailable' value not available on this system. Attempting to calculate available memory "
            "manually...\n");
        if (!(ptr = strstr(buf, "MemFree:")) || sscanf(ptr + 8, "%llu", &mem_free) != 1) {
            Log_error("system statistic error -- cannot get real memory free amount\n");
            goto error;
        }
        if (!(ptr = strstr(buf, "Buffers:")) || sscanf(ptr + 8, "%llu", &buffers) != 1)
            DEBUG("system statistic error -- cannot get real memory buffers amount\n");
        if (!(ptr = strstr(buf, "Cached:")) || sscanf(ptr + 7, "%llu", &cached) != 1)
            DEBUG("system statistic error -- cannot get real memory cache amount\n");
        if (!(ptr = strstr(buf, "SReclaimable:")) || sscanf(ptr + 13, "%llu", &slabreclaimable) != 1)
            DEBUG("system statistic error -- cannot get slab reclaimable memory amount\n");
        FILE *f = fopen("/proc/spl/kstat/zfs/arcstats", "r");
        if (f) {
            char line[STRLEN];
            while (fgets(line, sizeof(line), f)) {
                if (sscanf(line, "size %*d %llu", &zfsarcsize) == 1) {
                    break;
                }
            }
            fclose(f);
        }
        si->memory.usage.bytes = g_fixed_system_info.memory_size - zfsarcsize -
                                 (unsigned long long)(mem_free + buffers + cached + slabreclaimable) * 1024;
    }

    // Swap
    if (!(ptr = strstr(buf, "SwapTotal:")) || sscanf(ptr + 10, "%llu", &swap_total) != 1) {
        Log_error("system statistic error -- cannot get swap total amount\n");
        goto error;
    }
    if (!(ptr = strstr(buf, "SwapFree:")) || sscanf(ptr + 9, "%llu", &swap_free) != 1) {
        Log_error("system statistic error -- cannot get swap free amount\n");
        goto error;
    }
    si->swap.size = swap_total * 1024;
    si->swap.usage.bytes = (swap_total - swap_free) * 1024;

    return true;

error:
    si->memory.usage.bytes = 0ULL;
    si->swap.size = 0ULL;
    return false;
}

static double _usagePercent(unsigned long long previous, unsigned long long current, double total) {
    if (current < previous) {
        // The counter jumped back (observed for cpu wait metric on Linux 4.15) or wrapped
        return 0.;
    }
    return (double)(current - previous) / total * 100.;
}

static bool _getCpuUsateTime(CpuUsageTime *pCpuUsageTime) {
    int rv;

    char buf[8192];

    if (!pCpuUsageTime) {
        return false;
    }

    if (!file_readProc(buf, sizeof(buf), "stat", -1, -1, NULL)) {
        Log_error("system statistic error -- cannot read /proc/stat\n");
        return false;
    }

    rv = sscanf(buf,
                "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                &pCpuUsageTime->user,
                &pCpuUsageTime->nice,
                &pCpuUsageTime->system,
                &pCpuUsageTime->idle,
                &pCpuUsageTime->iowait,
                &pCpuUsageTime->hardirq,
                &pCpuUsageTime->softirq,
                &pCpuUsageTime->steal,
                &pCpuUsageTime->guest,
                &pCpuUsageTime->guest_nice);
    switch (rv) {
        case 4:
            // linux < 2.5.41
            pCpuUsageTime->iowait = 0;
            pCpuUsageTime->hardirq = 0;
            pCpuUsageTime->softirq = 0;
            pCpuUsageTime->steal = 0;
            pCpuUsageTime->guest = 0;
            pCpuUsageTime->guest_nice = 0;
            break;
        case 5:
            // linux >= 2.5.41
            pCpuUsageTime->hardirq = 0;
            pCpuUsageTime->softirq = 0;
            pCpuUsageTime->steal = 0;
            pCpuUsageTime->guest = 0;
            pCpuUsageTime->guest_nice = 0;
            break;
        case 7:
            // linux >= 2.6.0-test4
            pCpuUsageTime->steal = 0;
            pCpuUsageTime->guest = 0;
            pCpuUsageTime->guest_nice = 0;
            break;
        case 8:
            // linux 2.6.11
            pCpuUsageTime->guest = 0;
            pCpuUsageTime->guest_nice = 0;
            break;
        case 9:
            // linux >= 2.6.24
            pCpuUsageTime->guest_nice = 0;
            break;
        case 10:
            // linux >= 2.6.33
            break;
        default:
            Log_error("system statistic error -- cannot read cpu usage\n");
            return false;
    }

    pCpuUsageTime->total =
        pCpuUsageTime->user + pCpuUsageTime->nice + pCpuUsageTime->system + pCpuUsageTime->idle +
        pCpuUsageTime->iowait + pCpuUsageTime->hardirq + pCpuUsageTime->softirq +
        pCpuUsageTime->steal;  // Note: cpu_guest and cpu_guest_nice are included in user and nice already

    return true;
}

/**
 * This routine returns system/user CPU time in use.
 * @return: true if successful, false if failed (or not available)
 */
static bool used_system_cpu_sysdep(SystemInfo_T *si) {
    CpuUsageTime nowCpuUsageTime;

    if (!_getCpuUsateTime(&nowCpuUsageTime)) {
        goto error;
    }

    if (si->cpu.usage.old.total == 0) {
        si->cpu.usage.user = -1.;
        si->cpu.usage.nice = -1.;
        si->cpu.usage.system = -1.;
        si->cpu.usage.idle = -1.;
        si->cpu.usage.iowait = -1.;
        si->cpu.usage.hardirq = -1.;
        si->cpu.usage.softirq = -1.;
        si->cpu.usage.steal = -1.;
        si->cpu.usage.guest = -1.;
        si->cpu.usage.guest_nice = -1.;
    } else {
        double delta = nowCpuUsageTime.total - si->cpu.usage.old.total;
        si->cpu.usage.user = _usagePercent(si->cpu.usage.old.user - si->cpu.usage.old.guest,
                                           nowCpuUsageTime.user - nowCpuUsageTime.guest,
                                           delta);  // the guest (if available) is sub-statistics of user
        si->cpu.usage.nice = _usagePercent(si->cpu.usage.old.nice - si->cpu.usage.old.guest_nice,
                                           nowCpuUsageTime.nice - nowCpuUsageTime.guest_nice,
                                           delta);  // the guest_nice (if available) is sub-statistics of nice
        si->cpu.usage.system = _usagePercent(si->cpu.usage.old.system, nowCpuUsageTime.system, delta);
        si->cpu.usage.idle = _usagePercent(si->cpu.usage.old.idle, nowCpuUsageTime.idle, delta);
        si->cpu.usage.iowait = _usagePercent(si->cpu.usage.old.iowait, nowCpuUsageTime.iowait, delta);
        si->cpu.usage.hardirq = _usagePercent(si->cpu.usage.old.hardirq, nowCpuUsageTime.hardirq, delta);
        si->cpu.usage.softirq = _usagePercent(si->cpu.usage.old.softirq, nowCpuUsageTime.softirq, delta);
        si->cpu.usage.steal = _usagePercent(si->cpu.usage.old.steal, nowCpuUsageTime.steal, delta);
        si->cpu.usage.guest = _usagePercent(si->cpu.usage.old.guest, nowCpuUsageTime.guest, delta);
        si->cpu.usage.guest_nice = _usagePercent(si->cpu.usage.old.guest_nice, nowCpuUsageTime.guest_nice, delta);
    }

    memcpy(&si->cpu.usage.old, &nowCpuUsageTime, sizeof(nowCpuUsageTime));

    return true;

error:
    si->cpu.usage.user = 0.;
    si->cpu.usage.nice = 0.;
    si->cpu.usage.system = 0.;
    si->cpu.usage.iowait = 0.;
    si->cpu.usage.hardirq = 0.;
    si->cpu.usage.softirq = 0.;
    si->cpu.usage.steal = 0.;
    si->cpu.usage.guest = 0.;
    si->cpu.usage.guest_nice = 0.;
    return false;
}

/**
 * This routine returns filedescriptors statistics
 * @return: true if successful, false if failed (or not available)
 */
static bool used_system_filedescriptors_sysdep(SystemInfo_T *si) {
    bool rv = false;
    FILE *f = fopen("/proc/sys/fs/file-nr", "r");
    if (f) {
        char line[STRLEN];
        if (fgets(line, sizeof(line), f)) {
            if (sscanf(line,
                       "%lld %lld %lld\n",
                       &(si->filedescriptors.allocated),
                       &(si->filedescriptors.unused),
                       &(si->filedescriptors.maximum)) == 3) {
                rv = true;
            }
        }
        fclose(f);
    } else {
        DEBUG("system statistic error -- cannot open /proc/sys/fs/file-nr\n");
    }
    return rv;
}

bool init_system_info(SystemInfo_T *si) {
    memset(si, 0, sizeof(SystemInfo_T));
    gettimeofday(&si->collected, NULL);
    if (uname(&si->uname) < 0) {
        return false;
    }

    si->cpu.usage.user = -1.;
    si->cpu.usage.system = -1.;
    si->cpu.usage.iowait = -1.;

    return true;
}

bool update_system_info(SystemInfo_T *si) {
    if (getloadavg_sysdep(si->loadavg, 3) == -1) {
        goto error1;
    }

    if (!used_system_memory_sysdep(si)) {
        goto error2;
    }
    si->memory.usage.percent = g_fixed_system_info.memory_size > 0ULL
                                   ? (100. * (double)si->memory.usage.bytes / (double)g_fixed_system_info.memory_size)
                                   : 0.;
    si->swap.usage.percent = si->swap.size > 0ULL ? (100. * (double)si->swap.usage.bytes / (double)si->swap.size) : 0.;

    if (!used_system_cpu_sysdep(si)) {
        goto error3;
    }

    if (!used_system_filedescriptors_sysdep(si)) {
        goto error4;
    }

    return true;

error1:
    si->loadavg[0] = 0;
    si->loadavg[1] = 0;
    si->loadavg[2] = 0;
error2:
    si->memory.usage.bytes = 0ULL;
    si->memory.usage.percent = 0.;
    si->swap.usage.bytes = 0ULL;
    si->swap.usage.percent = 0.;
error3:
    si->cpu.usage.user = 0.;
    si->cpu.usage.system = 0.;
    si->cpu.usage.iowait = 0.;
error4:
    si->filedescriptors.allocated = 0LL;
    si->filedescriptors.unused = 0LL;
    si->filedescriptors.maximum = 0LL;

    return false;
}

unsigned long long getNowSingleCoreCpuTime(void) {
    CpuUsageTime nowCpuUsageTime;

    if (!_getCpuUsateTime(&nowCpuUsageTime)) {
        return 0;
    }

    return nowCpuUsageTime.total / g_fixed_system_info.cpu_count;
}
