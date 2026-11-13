#include "scheduler/Scheduler.h"

#include "util/Logger.h"

namespace arb {

void Scheduler::scheduleWithFixedDelay(std::string name, std::function<void()> task, std::chrono::milliseconds delay) {
    threads_.emplace_back([name = std::move(name), task = std::move(task), delay](std::stop_token stopToken) {
        while (!stopToken.stop_requested()) {
            try {
                task();
            } catch (const std::exception& e) {
                Logger::error("Scheduled task {} failed: {}", name, e.what());
            }
            std::this_thread::sleep_for(delay);
        }
    });
}

void Scheduler::join() {
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

}
