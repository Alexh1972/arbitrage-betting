#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace arb {

class Scheduler {
public:
    void scheduleWithFixedDelay(std::string name, std::function<void()> task, std::chrono::milliseconds delay);
    void join();

private:
    std::vector<std::jthread> threads_;
};

}
