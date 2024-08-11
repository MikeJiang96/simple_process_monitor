#include <cassert>
#include <cstdio>
#include <thread>

#include "simple_process_monitor/process_monitor.h"
#include "simple_process_monitor/system_info.h"

static void testSystemInfo() {
    using namespace std::chrono_literals;

    printf("CPU count is %d, RAM is %.1f MiB, hz is %.1f, page_size is %ld B\n",
           g_fixed_system_info.cpu_count,
           static_cast<double>(g_fixed_system_info.memory_size) / (1024 * 1024),
           g_fixed_system_info.hz,
           g_fixed_system_info.page_size);

    SystemInfo_T systemInfo;

    assert(init_system_info(&systemInfo));
    assert(update_system_info(&systemInfo));

    std::this_thread::sleep_for(1s);

    assert(update_system_info(&systemInfo));

    printf("CPU usage is %.1f %% / 100 %%\n", systemInfo.cpu.usage.user + systemInfo.cpu.usage.system);
    printf("\n");
}

static void testProcessMonitor() {
    using namespace simple_process_monitor;

    {
        ProcessMonitor pmAll(ALL_PROCESSES);

        pmAll.logTopCpu();
        pmAll.logTopRam();
    }

    {
        ProcessMonitor pmOneProcess(1);

        pmOneProcess.logTopCpu();
        pmOneProcess.logTopRam();
    }

    {
        ProcessMonitor pmInvalidProcess(1234567);

        pmInvalidProcess.logTopCpu();
        pmInvalidProcess.logTopRam();
    }
}

int main() {
    testSystemInfo();

    testProcessMonitor();

    return 0;
}
