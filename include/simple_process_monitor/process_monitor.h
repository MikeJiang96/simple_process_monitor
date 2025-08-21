#ifndef SIMPLE_PROCESS_MONITOR_PROCESS_MONITOR_H
#define SIMPLE_PROCESS_MONITOR_PROCESS_MONITOR_H

#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#include <simple_process_monitor/process_tree_wrapper.h>
#include <simple_process_monitor/system_info.h>

namespace simple_process_monitor {

class ProcessMonitor {
public:
    explicit ProcessMonitor(pid_t pid, std::chrono::seconds monitorInterval = std::chrono::seconds(1), int logCount = 5)
        : pid_(pid)
        , monitorInterval_(monitorInterval)
        , logCount_(logCount) {}

    ~ProcessMonitor() = default;

    ProcessMonitor(const ProcessMonitor &other) = delete;
    ProcessMonitor &operator=(const ProcessMonitor &other) = delete;

    void logTopCpu() const {
        logTopCpu(printf);
    }

    // First monitorInterval_ for top processes' CPU usages, second monitorInterval for their top threads' CPU usages.
    // So, threads' CPU usages will delay one monitorInterval_.
    // And total cost time is about two monitorInterval_.
    template <typename LOGGER_TYPE>
    void logTopCpu(LOGGER_TYPE &logger) const {
        TopProcessInfos topProcessInfos = collectTopInfo(TopInfoType::CPU);
        TopProcessThreadInfos topProcessThreadInfos;

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
                    topProcessThreadInfos[i] = pm.collectTopInfo(TopInfoType::CPU);
                });
            }

            for (auto &t : threads) {
                t.join();
            }

            logger("Top %lu of system processes' CPU usages\n", topProcessInfos.size());
        } else {
            logger("Top %lu of process pid %d's threads' CPU usages\n", topProcessInfos.size(), pid_);
        }

        logger("------------------------------------------------------------\n");

        for (unsigned long i = 0; i < topProcessInfos.size(); i++) {
            logger("%d  %.1f%%  %s\n",
                   topProcessInfos[i].pid,
                   topProcessInfos[i].cpuUsage,
                   topProcessInfos[i].cmdline.c_str());

            logger("------------------------------------------------------------\n");

            if (pid_ == ALL_PROCESSES) {
                for (auto &thread : topProcessThreadInfos[i]) {
                    logger("%d  %.1f%%  %s\n", thread.pid, thread.cpuUsage, thread.cmdline.c_str());
                }

                logger("------------------------------------------------------------\n");
            }
        }

        logger("\n");
    }

    void logTopRam() const {
        logTopRam(printf);
    }

    template <typename LOGGER_TYPE>
    void logTopRam(LOGGER_TYPE &logger) const {
        TopProcessInfos topProcessInfos = collectTopInfo(TopInfoType::RAM);

        if (pid_ == ALL_PROCESSES) {
            logger("Top %lu processes' RAM usages (total %.1f MiB)\n",
                   topProcessInfos.size(),
                   static_cast<double>(g_fixed_system_info.memory_size) / (1024 * 1024));
        } else {
            logger("Process pid %d's RAM usage (total %.1f MiB)\n",
                   pid_,
                   static_cast<double>(g_fixed_system_info.memory_size) / (1024.0 * 1024.0));
        }

        logger("------------------------------------------------------------\n");

        for (unsigned long i = 0; i < topProcessInfos.size(); i++) {
            if (pid_ != ALL_PROCESSES && i == 1) {
                break;
            }

            logger("%d  %.1f MiB  %s\n",
                   topProcessInfos[i].pid,
                   static_cast<double>(topProcessInfos[i].ramUsage) / (1024 * 1024),
                   topProcessInfos[i].cmdline.c_str());

            logger("------------------------------------------------------------\n");
        }

        logger("\n");
    }

private:
    using TopProcessInfos = std::vector<ProcessOrThreadInfo>;
    using TopProcessThreadInfos = std::vector<TopProcessInfos>;

    TopProcessInfos collectTopInfo(TopInfoType type) const {
        ProcessTreeWrapper processTreeWrapper{pid_};

        if (type == TopInfoType::CPU) {
            std::this_thread::sleep_for(monitorInterval_);
            processTreeWrapper.update();
        }

        return processTreeWrapper.getTopProcessInfos(type, logCount_);
    }

    const pid_t pid_;
    const std::chrono::seconds monitorInterval_;
    const int logCount_;
};

}  // namespace simple_process_monitor

#endif
