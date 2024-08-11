#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: busy_loop <busy_thread_nums>\n");
        exit(1);
    }

    const int threadNum = atoi(argv[1]);

    std::vector<std::thread> threads;

    for (int i = 0; i < threadNum; i++) {
        threads.emplace_back([]() {
            while (true)
                ;
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    return 0;
}
