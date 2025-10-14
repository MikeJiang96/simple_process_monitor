#ifndef SIMPLE_PROCESS_MONITOR_PROCESS_MONITOR_H
#define SIMPLE_PROCESS_MONITOR_PROCESS_MONITOR_H

#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <functional>
#include <string_view>
#include <thread>
#include <vector>

#include <simple_process_monitor/process_tree_wrapper.h>
#include <simple_process_monitor/system_info.h>

namespace simple_process_monitor {

class ProcessMonitor {
public:
    using LOGGER = std::function<int(std::string_view)>;

    explicit ProcessMonitor(pid_t pid, std::chrono::seconds monitorInterval = std::chrono::seconds(1), int logCount = 5)
        : pid_(pid)
        , monitorInterval_(monitorInterval)
        , logCount_(logCount) {}

    ~ProcessMonitor() = default;

    ProcessMonitor(const ProcessMonitor &other) = delete;
    ProcessMonitor &operator=(const ProcessMonitor &other) = delete;

    void logTopCpuToStdout() const {
        logTopCpu([](std::string_view s) {
            return ::printf("%s", s.data());
        });
    }

    // First monitorInterval_ for top processes' CPU usages, second monitorInterval for their top threads' CPU usages.
    // So, threads' CPU usages will delay one monitorInterval_.
    // And total cost time is about two monitorInterval_.
    void logTopCpu(LOGGER logger) const;

    void logTopRamToStdout() const {
        logTopRam([](std::string_view s) {
            return ::printf("%s", s.data());
        });
    }

    void logTopRam(LOGGER logger) const;

private:
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
