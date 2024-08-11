#ifndef PROCESS_TREE_WRAPPER_H
#define PROCESS_TREE_WRAPPER_H

#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "ProcessTree.h"

namespace simple_process_monitor {

enum class TopInfoType : uint8_t {
    CPU,
    RAM
};

struct ProcessOrThreadInfo {
    const pid_t pid;      // Can also be tid
    const int threadNum;  // Number of all threads in a process, even parsed from thread's procfs stat file
    const float cpuUsage;
    const unsigned long long ramUsage;
    const std::string cmdline;

    ProcessOrThreadInfo(pid_t pid_, int threadNum_, float cpuUsage_, unsigned long long ramUsage_, std::string cmdline_)
        : pid(pid_)
        , threadNum(threadNum_)
        , cpuUsage(cpuUsage_)
        , ramUsage(ramUsage_)
        , cmdline(std::move(cmdline_)) {}
};

class ProcessTreeWrapper {
public:
    explicit ProcessTreeWrapper(pid_t pid)
        : pid_(pid) {
        update();
    }

    ~ProcessTreeWrapper() {
        ProcessTree_delete(&pTree_, &treeSize_);
    }

    ProcessTreeWrapper(const ProcessTreeWrapper &) = delete;
    ProcessTreeWrapper &operator=(const ProcessTreeWrapper &) = delete;

    void update() {
        const int treeSize = ProcessTree_init(&pTree_, &treeSize_, static_cast<int>(pid_));

        if (pid_ == ALL_PROCESSES) {
            assert(treeSize >= 0);
        }
    }

    [[nodiscard]] std::vector<ProcessOrThreadInfo> getTopProcessInfos(TopInfoType type, int count) const;

private:
    template <typename T>
    std::vector<ProcessOrThreadInfo> getTopProcessInfos(T &maxPQ, int count) const;

    const pid_t pid_;

    ProcessTree_T *pTree_ = nullptr;
    int treeSize_ = 0;
};

}  // namespace simple_process_monitor

#endif
