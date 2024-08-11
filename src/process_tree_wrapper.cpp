#include "process_tree_wrapper.h"

#include <queue>

namespace simple_process_monitor {

std::vector<ProcessOrThreadInfo> ProcessTreeWrapper::getTopProcessInfos(TopInfoType type, int count) const {
    if (treeSize_ <= 0) {
        return {};
    }

    if (type == TopInfoType::CPU) {
        auto processCpuCompare = [](const ProcessTree_T *p1, const ProcessTree_T *p2) {
            return p1->cpu.usage.self < p2->cpu.usage.self;
        };

        using ProcessCpuPQ =
            std::priority_queue<const ProcessTree_T *, std::vector<const ProcessTree_T *>, decltype(processCpuCompare)>;

        ProcessCpuPQ pq{processCpuCompare};

        return getTopProcessInfos(pq, count);
    }

    if (type == TopInfoType::RAM) {
        auto processRamCompare = [](const ProcessTree_T *p1, const ProcessTree_T *p2) {
            return p1->memory.usage < p2->memory.usage;
        };

        using ProcessRamPQ =
            std::priority_queue<const ProcessTree_T *, std::vector<const ProcessTree_T *>, decltype(processRamCompare)>;

        ProcessRamPQ pq{processRamCompare};

        return getTopProcessInfos(pq, count);
    }

    return {};
}

template <typename T>
std::vector<ProcessOrThreadInfo> ProcessTreeWrapper::getTopProcessInfos(T &maxPQ, int count) const {
    std::vector<ProcessOrThreadInfo> ret;

    for (int i = 0; i < treeSize_; i++) {
        // Skip the virtual root
        if (pTree_[i].pid > 0) {
            maxPQ.push(&pTree_[i]);
        }
    }

    count = std::min(treeSize_, count);

    for (int i = 0; i < count; i++) {
        const ProcessTree_T *pProcess = maxPQ.top();

        if (pProcess->cmdline) {
            ret.emplace_back(pProcess->pid,
                             pProcess->threads.self,
                             pProcess->cpu.usage.self,
                             pProcess->memory.usage,
                             pProcess->cmdline);
        } else {
            ret.emplace_back(
                pProcess->pid, pProcess->threads.self, pProcess->cpu.usage.self, pProcess->memory.usage, "(null)");
        }

        maxPQ.pop();
    }

    return ret;
}

}  // namespace simple_process_monitor
