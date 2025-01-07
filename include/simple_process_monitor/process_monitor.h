#ifndef SIMPLE_PROCESS_MONITOR_PROCESS_MONITOR_H
#define SIMPLE_PROCESS_MONITOR_PROCESS_MONITOR_H

#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <vector>

#include "process_tree_wrapper.h"

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

    void logTopCpu() const;

    void logTopRam() const;

private:
    using TopProcessInfos = std::vector<ProcessOrThreadInfo>;
    using OneProcessThreadInfos = TopProcessInfos;
    using TopProcessThreadInfos = std::vector<OneProcessThreadInfos>;
    using LogInfo = std::pair<TopProcessInfos, TopProcessThreadInfos>;

    void collectTopInfo(TopInfoType type, TopProcessInfos &threadInfos) const;

    const pid_t pid_;
    const std::chrono::seconds monitorInterval_;
    const int logCount_;
};

}  // namespace simple_process_monitor

#endif
