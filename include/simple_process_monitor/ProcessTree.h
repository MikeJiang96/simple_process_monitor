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

#ifndef PROCESS_TREE_H
#define PROCESS_TREE_H

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALL_PROCESSES (-1)

typedef struct ProcessTree_T {
    bool visited;
    bool zombie;
    pid_t pid;
    pid_t ppid;
    int parent;

    struct {
        int uid;
        int euid;
        int gid;
    } cred;

    struct {
        struct {
            float self;
            float children;
        } usage;

        double time;
    } cpu;

    struct {
        int self;
        int children;
    } threads;

    struct {
        int count;
        int total;
        int *list;
    } children;

    struct {
        unsigned long long usage;
        unsigned long long usage_total;
    } memory;

    struct {
        unsigned long long time;
        long long bytes;
        long long bytesPhysical;
        long long operations;
    } read;

    struct {
        unsigned long long time;
        long long bytes;
        long long bytesPhysical;
        long long operations;
    } write;

    time_t uptime;
    char *cmdline;
    char *secattr;

    struct {
        long long usage;
        long long usage_total;

        struct {
            long long soft;
            long long hard;
        } limit;
    } filedescriptors;

    double time;
} ProcessTree_T;

/**
 * Initialize the process tree
 * @return The process tree size or -1 if failed
 */
int ProcessTree_init(ProcessTree_T **ppTree, int *pTreeSize, int pid);

/**
 * Delete the process tree
 */
void ProcessTree_delete(ProcessTree_T **ppTree, int *pTreeSize);

#ifdef __cplusplus
}
#endif

#endif
