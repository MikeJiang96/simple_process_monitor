#include "simple_process_monitor/process_monitor.h"

#include <cstdio>
#include <thread>

#include "simple_process_monitor/system_info.h"

namespace simple_process_monitor {

void ProcessMonitor::logTopCpu() const {
    LogInfo logInfo;
    TopProcessInfos &topProcessInfos = logInfo.first;
    TopProcessThreadInfos &topProcessThreadInfos = logInfo.second;

    collectTopInfo(TopInfoType::CPU, topProcessInfos);

    if (pid_ == ALL_PROCESSES) {
        topProcessThreadInfos.resize(topProcessInfos.size());

        std::vector<std::thread> threads;

        for (unsigned long i = 0; i < topProcessInfos.size(); i++) {
            auto &process = topProcessInfos[i];

            threads.emplace_back([pid = process.pid,
                                  monitorInterval = monitorInterval_,
                                  logCount = logCount_,
                                  &topProcessThreadInfos,
                                  i]() {
                ProcessMonitor pm{pid, monitorInterval, logCount};
                pm.collectTopInfo(TopInfoType::CPU, topProcessThreadInfos[i]);
            });
        }

        for (auto &t : threads) {
            t.join();
        }

        printf("Top %lu processes' CPU usages\n", topProcessInfos.size());
    } else {
        printf("Top %lu process pid %d's threads' CPU usages\n", topProcessInfos.size(), pid_);
    }

    printf("------------------------------------------------------------\n");

    for (unsigned long i = 0; i < topProcessInfos.size(); i++) {
        printf("%d  %.1f %%  %s\n",
               topProcessInfos[i].pid,
               topProcessInfos[i].cpuUsage,
               topProcessInfos[i].cmdline.c_str());

        printf("------------------------------------------------------------\n");

        if (pid_ == ALL_PROCESSES) {
            for (auto &thread : topProcessThreadInfos[i]) {
                printf("%d  %.1f %%  %s\n", thread.pid, thread.cpuUsage, thread.cmdline.c_str());
            }

            printf("------------------------------------------------------------\n");
        }
    }

    printf("\n");
}

void ProcessMonitor::logTopRam() const {
    TopProcessInfos topProcessInfos;

    collectTopInfo(TopInfoType::RAM, topProcessInfos);

    if (pid_ == ALL_PROCESSES) {
        printf("Top %lu processes' RAM usages (total %.1f MiB)\n",
               topProcessInfos.size(),
               static_cast<double>(g_fixed_system_info.memory_size) / (1024 * 1024));
    } else {
        printf("Process pid %d's RAM usage (total %.1f MiB)\n",
               pid_,
               static_cast<double>(g_fixed_system_info.memory_size) / (1024.0 * 1024.0));
    }

    printf("------------------------------------------------------------\n");

    for (unsigned long i = 0; i < topProcessInfos.size(); i++) {
        if (pid_ != ALL_PROCESSES && i == 1) {
            break;
        }

        printf("%d  %.1f MiB  %s\n",
               topProcessInfos[i].pid,
               static_cast<double>(topProcessInfos[i].ramUsage) / (1024 * 1024),
               topProcessInfos[i].cmdline.c_str());

        printf("------------------------------------------------------------\n");
    }

    printf("\n");
}

void ProcessMonitor::collectTopInfo(TopInfoType type, TopProcessInfos &infos) const {
    ProcessTreeWrapper processTreeWrapper{pid_};

    if (type == TopInfoType::CPU) {
        std::this_thread::sleep_for(monitorInterval_);

        processTreeWrapper.update();
    }

    infos = processTreeWrapper.getTopProcessInfos(type, logCount_);
}

}  // namespace simple_process_monitor
