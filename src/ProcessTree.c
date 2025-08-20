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

#include "simple_process_monitor/ProcessTree.h"

#include <assert.h>
#include <dirent.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

#include "simple_process_monitor/system_info.h"
#include "util/Mem.h"
#include "util/Str.h"
#include "util/StringBuffer.h"
#include "util/debug.h"
#include "util/file.h"
#include "util/time.h"

/**
 *  General purpose /proc methods.
 *
 *  @file
 */

/* ------------------------------------------------------------- Definitions */

/* ----------------------------------------------------------------- Private */

/**
 * Get system start time
 * @return seconds since unix epoch
 */
static time_t _getStartTime(void) {
    struct sysinfo info;

    sysinfo(&info);

    return Time_now() - info.uptime;
}

static void _delete(ProcessTree_T **pt, int *size) {
    assert(pt);
    ProcessTree_T *_pt = *pt;
    if (_pt) {
        for (int i = 0; i < *size; i++) {
            FREE(_pt[i].cmdline);
            FREE(_pt[i].children.list);
            FREE(_pt[i].secattr);
        }
        FREE(_pt);
        *pt = NULL;
        *size = 0;
    }
}

/**
 * Search a leaf in the processtree
 * @param pid  pid of the process
 * @param pt  processtree
 * @param treesize  size of the processtree
 * @return process index if succeeded otherwise -1
 */
static int _findProcess(int pid, ProcessTree_T *pt, int size) {
    if (size > 0) {
        for (int i = 0; i < size; i++)
            if (pid == pt[i].pid)
                return i;
    }
    return -1;
}

/**
 * Fill data in the process tree by recursively walking through it
 * @param pt process tree
 * @param i process index
 */
static void _fillProcessTree(ProcessTree_T *pt, int index) {
    if (!pt[index].visited) {
        pt[index].visited = true;
        pt[index].children.total = pt[index].children.count;
        pt[index].threads.children = 0;
        pt[index].cpu.usage.children = 0.;
        pt[index].memory.usage_total = pt[index].memory.usage;
        pt[index].filedescriptors.usage_total = pt[index].filedescriptors.usage;
        for (int i = 0; i < pt[index].children.count; i++) {
            _fillProcessTree(pt, pt[index].children.list[i]);
        }
        if (pt[index].parent != -1 && pt[index].parent != index) {
            ProcessTree_T *parent_pt = &pt[pt[index].parent];
            parent_pt->children.total += pt[index].children.total;
            parent_pt->threads.children += (pt[index].threads.self > 1 ? pt[index].threads.self : 1) +
                                           (pt[index].threads.children > 0 ? pt[index].threads.children : 0);
            if (pt[index].cpu.usage.self >= 0) {
                parent_pt->cpu.usage.children += pt[index].cpu.usage.self;
            }
            if (pt[index].cpu.usage.children >= 0) {
                parent_pt->cpu.usage.children += pt[index].cpu.usage.children;
            }
            parent_pt->memory.usage_total += pt[index].memory.usage_total;
            parent_pt->filedescriptors.usage_total += pt[index].filedescriptors.usage_total;
        }
    }
}

static int _initprocesstree_sysdep(ProcessTree_T **reference, int pid);

/* ------------------------------------------------------------------ Public */

/**
 * Initialize the process tree
 * @return treesize >= 0 if succeeded otherwise < 0
 */
int ProcessTree_init(ProcessTree_T **ppTree, int *pTreeSize, int pid) {
    ProcessTree_T *oldptree = *ppTree;
    int oldptreesize = *pTreeSize;
    if (oldptree) {
        *ppTree = NULL;
        *pTreeSize = 0;
        // We need only process's cpu.time from the old ptree, so free dynamically allocated parts which we don't need
        // before initializing new ptree (so the memory can be reused, otherwise the memory footprint will hold two
        // ptrees)
        for (int i = 0; i < oldptreesize; i++) {
            FREE(oldptree[i].cmdline);
            FREE(oldptree[i].children.list);
            FREE(oldptree[i].secattr);
        }
    }

    double time_prev = oldptree ? oldptree->time : 0.;

    if ((*pTreeSize = _initprocesstree_sysdep(ppTree, pid)) <= 0 || !(*ppTree)) {
        DEBUG("System statistic -- cannot initialize the process tree -- process resource monitoring disabled\n");
        if (oldptree)
            _delete(ppTree, pTreeSize);
        return -1;
    }

    int root = -1;  // Main process. Not all systems have main process with PID 1 (such as Solaris zones and FreeBSD
                    // jails), so we try to find process which is parent of itself
    ProcessTree_T *pt = *ppTree;

    // Or simply get the monotonic clock?
    pt->time = getNowSingleCoreCpuTime();

    double time_delta = pt->time - time_prev;

    for (int i = 0; i < (volatile int)*pTreeSize; i++) {
        pt[i].cpu.usage.self = -1;
        if (oldptree) {
            int oldentry = _findProcess(pt[i].pid, oldptree, oldptreesize);
            if (oldentry != -1) {
                if (g_fixed_system_info.cpu_count > 0 && time_delta > 0 && oldptree[oldentry].cpu.time >= 0 &&
                    pt[i].cpu.time >= oldptree[oldentry].cpu.time) {
                    pt[i].cpu.usage.self = 100. * (pt[i].cpu.time - oldptree[oldentry].cpu.time) / time_delta;

                    // Adjust if we are collecting one process' thread infos
                    if (pid != ALL_PROCESSES) {
                        pt[i].cpu.usage.self = pt[i].cpu.usage.self < 100. ? pt[i].cpu.usage.self : 100.;
                    }
                }
            }
        }
        // Note: on DragonFly, main process is swapper with pid 0 and ppid -1, so take also this case into consideration
        if ((pt[i].pid == pt[i].ppid) || (pt[i].ppid == -1)) {
            root = pt[i].parent = i;
        } else {
            // Find this process's parent when not creating thread tree
            if (pid == ALL_PROCESSES) {
                int parent = _findProcess(pt[i].ppid, pt, *pTreeSize);
                if (parent == -1) {
                    /* Parent process wasn't found - on Linux this is normal: main process with PID 0 is not listed,
                     * similarly in FreeBSD jail. We create virtual process entry for missing parent so we can have full
                     * tree-like structure with root. */
                    parent = (*pTreeSize)++;
                    pt = RESIZE(*ppTree, (*pTreeSize) * sizeof(ProcessTree_T));
                    memset(&pt[parent], 0, sizeof(ProcessTree_T));
                    root = pt[parent].ppid = pt[parent].pid = pt[i].ppid;
                }
                pt[i].parent = parent;
                // Connect the child (this process) to the parent
                RESIZE(pt[parent].children.list, sizeof(int) * (pt[parent].children.count + 1));
                pt[parent].children.list[pt[parent].children.count] = i;
                pt[parent].children.count++;
            }
        }
    }
    FREE(oldptree);  // Free the rest of old ptree

    if (pid == ALL_PROCESSES) {
        if (root == -1) {
            DEBUG("System statistic error -- cannot find root process id\n");
            _delete(ppTree, pTreeSize);
            return -1;
        }

        _fillProcessTree(pt, root);
    }

    return *pTreeSize;
}

/**
 * Delete the process tree
 */
void ProcessTree_delete(ProcessTree_T **ppTree, int *pTreeSize) {
    _delete(ppTree, pTreeSize);
}

/**
 *  System dependent resource data collection code for Linux.
 *
 *  @file
 */

/* ------------------------------------------------------------- Definitions */

static struct {
    int hasIOStatistics;  // True if /proc/<PID>/io is present
} _statistics = {};

typedef struct Proc_T {
    StringBuffer_T name;

    struct {
        int pid;
        int tid;
        int ppid;
        int uid;
        int euid;
        int gid;
        char item_state;
        long item_cutime;
        long item_cstime;
        long item_rss;
        int item_threads;
        unsigned long item_utime;
        unsigned long item_stime;
        unsigned long long item_starttime;

        struct {
            unsigned long long bytes;
            unsigned long long bytesPhysical;
            unsigned long long operations;
        } read;

        struct {
            unsigned long long bytes;
            unsigned long long bytesPhysical;
            unsigned long long operations;
        } write;

        struct {
            long long open;

            struct {
                long long soft;
                long long hard;
            } limit;
        } filedescriptors;

        char secattr[STRLEN];
    } data;
} *Proc_T;

/* --------------------------------------- Static constructor and destructor */

static void __attribute__((constructor)) _constructor(void) {
    struct stat sb;
    _statistics.hasIOStatistics = stat("/proc/self/io", &sb) == 0 ? true : false;
}

/* ----------------------------------------------------------------- Private */

// parse /proc/PID/stat or /proc/PID/task/TID/stat
static bool _parseProcPidStat(Proc_T proc) {
    char buf[8192];
    char *tmp = NULL;
    if (!file_readProc(buf, sizeof(buf), "stat", proc->data.pid, proc->data.tid, NULL)) {
        DEBUG("system statistic error -- cannot read /proc/%d/stat\n", proc->data.pid);
        return false;
    }
    // Skip the process name (can have multiple words)
    if (!(tmp = strrchr(buf, ')'))) {
        DEBUG("system statistic error -- file /proc/%d/stat parse error\n", proc->data.pid);
        return false;
    }
    if (sscanf(tmp + 2,
               "%c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %ld %ld %*d %*d %d %*u %llu %*u %ld %*u %*u %*u %*u "
               "%*u %*u %*u %*u %*u %*u %*u %*u %*u %*d %*d\n",
               &(proc->data.item_state),
               &(proc->data.ppid),
               &(proc->data.item_utime),
               &(proc->data.item_stime),
               &(proc->data.item_cutime),
               &(proc->data.item_cstime),
               &(proc->data.item_threads),
               &(proc->data.item_starttime),
               &(proc->data.item_rss)) != 9) {
        DEBUG("system statistic error -- file /proc/%d/stat parse error\n", proc->data.pid);
        return false;
    }
    return true;
}

// parse /proc/PID/status
static bool _parseProcPidStatus(Proc_T proc) {
    char buf[4096];
    char *tmp = NULL;
    if (!file_readProc(buf, sizeof(buf), "status", proc->data.pid, proc->data.tid, NULL)) {
        DEBUG("system statistic error -- cannot read /proc/%d/status\n", proc->data.pid);
        return false;
    }
    if (!(tmp = strstr(buf, "Uid:"))) {
        DEBUG("system statistic error -- cannot find process uid\n");
        return false;
    }
    if (sscanf(tmp + 4, "\t%d\t%d", &(proc->data.uid), &(proc->data.euid)) != 2) {
        DEBUG("system statistic error -- cannot read process uid\n");
        return false;
    }
    if (!(tmp = strstr(buf, "Gid:"))) {
        DEBUG("system statistic error -- cannot find process gid\n");
        return false;
    }
    if (sscanf(tmp + 4, "\t%d", &(proc->data.gid)) != 1) {
        DEBUG("system statistic error -- cannot read process gid\n");
        return false;
    }
    return true;
}

// parse /proc/PID/io
static bool _parseProcPidIO(Proc_T proc) {
    char buf[4096];
    char *tmp = NULL;
    if (_statistics.hasIOStatistics) {
        if (file_readProc(buf, sizeof(buf), "io", proc->data.pid, proc->data.tid, NULL)) {
            // read bytes (total)
            if (!(tmp = strstr(buf, "rchar:"))) {
                DEBUG("system statistic error -- cannot find process read bytes\n");
                return false;
            }
            if (sscanf(tmp + 6, "\t%llu", &(proc->data.read.bytes)) != 1) {
                DEBUG("system statistic error -- cannot get process read bytes\n");
                return false;
            }
            // write bytes (total)
            if (!(tmp = strstr(tmp, "wchar:"))) {
                DEBUG("system statistic error -- cannot find process write bytes\n");
                return false;
            }
            if (sscanf(tmp + 6, "\t%llu", &(proc->data.write.bytes)) != 1) {
                DEBUG("system statistic error -- cannot get process write bytes\n");
                return false;
            }
            // read operations
            if (!(tmp = strstr(tmp, "syscr:"))) {
                DEBUG("system statistic error -- cannot find process read system calls count\n");
                return false;
            }
            if (sscanf(tmp + 6, "\t%llu", &(proc->data.read.operations)) != 1) {
                DEBUG("system statistic error -- cannot get process read system calls count\n");
                return false;
            }
            // write operations
            if (!(tmp = strstr(tmp, "syscw:"))) {
                DEBUG("system statistic error -- cannot find process write system calls count\n");
                return false;
            }
            if (sscanf(tmp + 6, "\t%llu", &(proc->data.write.operations)) != 1) {
                DEBUG("system statistic error -- cannot get process write system calls count\n");
                return false;
            }
            // read bytes (physical I/O)
            if (!(tmp = strstr(tmp, "read_bytes:"))) {
                DEBUG("system statistic error -- cannot find process physical read bytes\n");
                return false;
            }
            if (sscanf(tmp + 11, "\t%llu", &(proc->data.read.bytesPhysical)) != 1) {
                DEBUG("system statistic error -- cannot get process physical read bytes\n");
                return false;
            }
            // write bytes (physical I/O)
            if (!(tmp = strstr(tmp, "write_bytes:"))) {
                DEBUG("system statistic error -- cannot find process physical write bytes\n");
                return false;
            }
            if (sscanf(tmp + 12, "\t%llu", &(proc->data.write.bytesPhysical)) != 1) {
                DEBUG("system statistic error -- cannot get process physical write bytes\n");
                return false;
            }
        } else {
            // file_readProc() already printed a DEBUG() message
            // return false;
            // sometimes no io data is available, this is not a problem.
            return true;
        }
    }
    return true;
}

// parse /proc/PID/cmdline or /proc/PID/task/TID/stat
static bool _parseProcPidCmdline(Proc_T proc) {
    if (proc->data.tid == -1) {
        char filename[STRLEN];
        // Try to collect the command-line from the procfs cmdline (user-space processes)
        snprintf(filename, sizeof(filename), "/proc/%d/cmdline", proc->data.pid);
        FILE *f = fopen(filename, "r");
        if (!f) {
            DEBUG("system statistic error -- cannot open /proc/%d/cmdline: %s\n", proc->data.pid, STRERROR);
            return false;
        }
        size_t n;
        char buf[STRLEN] = {};
        while ((n = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
            // The cmdline file contains argv elements/strings separated by '\0' => join the string
            for (size_t i = 0; i < n; i++) {
                if (buf[i] == 0)
                    StringBuffer_append(proc->name, " ");
                else
                    StringBuffer_append(proc->name, "%c", buf[i]);
            }
        }
        fclose(f);
        StringBuffer_trim(proc->name);
    }
    // Fallback to procfs stat process name if cmdline was empty (even kernel-space processes have information here),
    // or if we want to get a thread's name
    if (!StringBuffer_length(proc->name)) {
        char buffer[8192];
        char *tmp = NULL;
        char *procname = NULL;
        if (!file_readProc(buffer, sizeof(buffer), "stat", proc->data.pid, proc->data.tid, NULL)) {
            DEBUG("system statistic error -- cannot read /proc/%d/stat or /proc/%d/task/%d/stat\n",
                  proc->data.pid,
                  proc->data.pid,
                  proc->data.tid);
            return false;
        }
        if (!(tmp = strrchr(buffer, ')'))) {
            DEBUG("system statistic error -- file /proc/%d/stat or /proc/%d/task/%d/stat parse error\n",
                  proc->data.pid,
                  proc->data.pid,
                  proc->data.tid);
            return false;
        }
        *tmp = 0;
        if (!(procname = strchr(buffer, '('))) {
            DEBUG("system statistic error -- file /proc/%d/stat or /proc/%d/task/%d/stat parse error\n",
                  proc->data.pid,
                  proc->data.pid,
                  proc->data.tid);
            return false;
        }
        StringBuffer_append(proc->name, "%s", procname + 1);
    }

    return true;
}

// parse /proc/PID/attr/current
static bool _parseProcPidAttrCurrent(Proc_T proc) {
    if (file_readProc(
            proc->data.secattr, sizeof(proc->data.secattr), "attr/current", proc->data.pid, proc->data.tid, NULL)) {
        Str_trim(proc->data.secattr);
        return true;
    }
    return false;
}

// count entries in /proc/PID/fd
static bool _parseProcFdCount(Proc_T proc) {
    char path[PATH_MAX] = {};
    unsigned long long file_count = 0;

    snprintf(path, sizeof(path), "/proc/%d/fd", proc->data.pid);
    DIR *dirp = opendir(path);
    if (!dirp) {
        DEBUG("system statistic error -- opendir %s: %s\n", path, STRERROR);
        return false;
    }
    errno = 0;
    while (readdir(dirp) != NULL) {
        // count everything
        file_count++;
    }
    // do not closedir() until readdir errno has been evaluated
    if (errno) {
        DEBUG("system statistic error -- cannot iterate %s: %s\n", path, STRERROR);
        closedir(dirp);
        return false;
    }
    closedir(dirp);
    // assert at least '.' and '..' have been found
    if (file_count < 2) {
        DEBUG("system statistic error -- cannot find basic entries in %s\n", path);
        return false;
    }
    // subtract entries '.' and '..'
    proc->data.filedescriptors.open = file_count - 2;

    // get process's limits
    snprintf(path, sizeof(path), "/proc/%d/limits", proc->data.pid);
    FILE *f = fopen(path, "r");
    if (f) {
        int softLimit;
        int hardLimit;
        char line[STRLEN];
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Max open files %d %d", &softLimit, &hardLimit) == 2) {
                proc->data.filedescriptors.limit.soft = softLimit;
                proc->data.filedescriptors.limit.hard = hardLimit;
                break;
            }
        }
        fclose(f);
    } else {
        DEBUG("system statistic error -- cannot open %s\n", path);
        return false;
    }

    return true;
}

/* ------------------------------------------------------------------ Public */

/**
 * Read all processes of the proc files system to initialize the process tree
 * @param reference reference of ProcessTree
 * @param pflags Process engine flags
 * @return treesize > 0 if succeeded otherwise 0
 */
static int _initprocesstree_sysdep(ProcessTree_T **reference, int pid) {
    assert(reference);

    // Find all processes in the /proc directory, or all threads in the /proc/<pid>/task directory
    glob_t globbuf;

    if (pid == ALL_PROCESSES) {
        int rv = glob("/proc/[0-9]*", 0, NULL, &globbuf);

        if (rv) {
            Log_error("system statistic error -- glob failed: %d (%s)\n", rv, STRERROR);
            return 0;
        }
    } else {
        char pattern[128];

        snprintf(pattern, sizeof(pattern), "/proc/%d/task/[0-9]*", pid);

        int rv = glob(pattern, 0, NULL, &globbuf);

        if (rv) {
            DEBUG("system statistic error -- glob %s failed: %d (%s)\n", pattern, rv, STRERROR);
            return 0;
        }
    }

    ProcessTree_T *pt = CALLOC(sizeof(ProcessTree_T), globbuf.gl_pathc);

    int count = 0;
    struct Proc_T proc = {.name = StringBuffer_create(64)};
    time_t starttime = _getStartTime();
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        if (pid == ALL_PROCESSES) {
            proc.data.pid = atoi(globbuf.gl_pathv[i] + 6);  // Skip "/proc/"
            proc.data.tid = -1;
        } else {
            int pid_copy = pid;
            int digits = 0;

            while (pid_copy > 0) {
                pid_copy /= 10;
                digits++;
            }

            // DO NOT MODIFY pid!
            proc.data.pid = pid;
            proc.data.tid = atoi(globbuf.gl_pathv[i] + 6 + digits + 6);  // Skip "/proc/<pid>/task/"
        }

        if (_parseProcPidStat(&proc) && _parseProcPidStatus(&proc) && _parseProcPidIO(&proc) &&
            _parseProcPidCmdline(&proc)) {
            // Non-mandatory statistics (may not exist)
            _parseProcFdCount(&proc);
            _parseProcPidAttrCurrent(&proc);
            // Set the data in ptree only if all process related reads succeeded (prevent partial data in the case that
            // continue was called during data collecting)
            if (pid == ALL_PROCESSES) {
                pt[count].pid = proc.data.pid;
            } else {
                pt[count].pid = proc.data.tid;
            }

            pt[count].ppid = proc.data.ppid;
            pt[count].cred.uid = proc.data.uid;
            pt[count].cred.euid = proc.data.euid;
            pt[count].cred.gid = proc.data.gid;
            pt[count].threads.self = proc.data.item_threads;
            pt[count].uptime = starttime > 0
                                   ? ((Time_milli() / 100.) / 10. -
                                      (starttime + (time_t)(proc.data.item_starttime / g_fixed_system_info.hz)))
                                   : 0;
            pt[count].cpu.time = (double)(proc.data.item_utime + proc.data.item_stime) / g_fixed_system_info.hz *
                                 100.;  // jiffies -> seconds = 1/hz
            pt[count].memory.usage =
                (unsigned long long)proc.data.item_rss * (unsigned long long)g_fixed_system_info.page_size;
            pt[count].read.bytes = proc.data.read.bytes;
            pt[count].read.bytesPhysical = proc.data.read.bytesPhysical;
            pt[count].read.operations = proc.data.read.operations;
            pt[count].write.bytes = proc.data.write.bytes;
            pt[count].write.bytesPhysical = proc.data.write.bytesPhysical;
            pt[count].write.operations = proc.data.write.operations;
            pt[count].read.time = pt[count].write.time = Time_milli();
            pt[count].zombie = proc.data.item_state == 'Z' ? true : false;
            pt[count].cmdline = Str_dup(StringBuffer_toString(proc.name));
            pt[count].secattr = Str_dup(proc.data.secattr);
            pt[count].filedescriptors.usage = proc.data.filedescriptors.open;
            pt[count].filedescriptors.limit.soft = proc.data.filedescriptors.limit.soft;
            pt[count].filedescriptors.limit.hard = proc.data.filedescriptors.limit.hard;
            count++;
            // Clear
            memset(&proc.data, 0, sizeof(proc.data));
            StringBuffer_clear(proc.name);
        }
    }
    StringBuffer_free(&(proc.name));

    *reference = pt;
    globfree(&globbuf);

    return count;
}
