#include <simple_process_monitor/process_monitor.h>

#include <cstdio>

namespace simple_process_monitor {

template <typename... Ts>
static void formatAndLog(ProcessMonitor::LOGGER &logger, const char *fmt, Ts... args) {
    static constexpr size_t kBufSize = 4096;

    char buf[kBufSize];

    const int n = std::snprintf(buf, sizeof(buf), fmt, args...);

    if (n < 0) {
        logger("formatAndLog: snprintf formatting error");
        return;
    }

    if (static_cast<size_t>(n) >= sizeof(buf)) {
        logger(std::string(buf) + " [TRUNCATED]");
    } else {
        logger(std::string_view(buf, static_cast<size_t>(n)));
    }
}

void ProcessMonitor::logTopCpu(LOGGER logger) const {
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

        formatAndLog(logger, "Top %lu of system processes' CPU usages\n", topProcessInfos.size());
    } else {
        formatAndLog(logger, "Top %lu of process pid %d's threads' CPU usages\n", topProcessInfos.size(), pid_);
    }

    logger("------------------------------------------------------------\n");

    for (unsigned long i = 0; i < topProcessInfos.size(); i++) {
        formatAndLog(logger,
                     "%d  %.1f%%  %s\n",
                     topProcessInfos[i].pid,
                     topProcessInfos[i].cpuUsage,
                     topProcessInfos[i].cmdline.c_str());

        logger("------------------------------------------------------------\n");

        if (pid_ == ALL_PROCESSES) {
            for (auto &thread : topProcessThreadInfos[i]) {
                formatAndLog(logger, "%d  %.1f%%  %s\n", thread.pid, thread.cpuUsage, thread.cmdline.c_str());
            }

            logger("------------------------------------------------------------\n");
        }
    }

    logger("\n");
}

void ProcessMonitor::logTopRam(LOGGER logger) const {
    TopProcessInfos topProcessInfos = collectTopInfo(TopInfoType::RAM);

    if (pid_ == ALL_PROCESSES) {
        formatAndLog(logger,
                     "Top %lu processes' RAM usages (total %.1f MiB)\n",
                     topProcessInfos.size(),
                     static_cast<double>(g_fixed_system_info.memory_size) / (1024 * 1024));
    } else {
        formatAndLog(logger,
                     "Process pid %d's RAM usage (total %.1f MiB)\n",
                     pid_,
                     static_cast<double>(g_fixed_system_info.memory_size) / (1024.0 * 1024.0));
    }

    logger("------------------------------------------------------------\n");

    for (unsigned long i = 0; i < topProcessInfos.size(); i++) {
        if (pid_ != ALL_PROCESSES && i == 1) {
            break;
        }

        formatAndLog(logger,
                     "%d  %.1f MiB  %s\n",
                     topProcessInfos[i].pid,
                     static_cast<double>(topProcessInfos[i].ramUsage) / (1024 * 1024),
                     topProcessInfos[i].cmdline.c_str());

        logger("------------------------------------------------------------\n");
    }

    logger("\n");
}

}  // namespace simple_process_monitor
